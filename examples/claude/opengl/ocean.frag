#version 120
// ocean.frag - open sea under a procedural sky, ray marched per pixel.
//
// Scinc (ocean.cpp) owns the physics that is the same for every pixel and
// passes it in as uniforms: the Gerstner wave set built from the wind, the
// camera, the sun and moon directions, and the sun colour and sky ambient it
// integrates through the atmosphere once per frame. This shader does what is
// different per pixel:
// Each frame is drawn in two passes. Pass 1 renders the sky (atmosphere,
// night, clouds) into a small 256x96 panorama that Scinc copies into a
// texture; pass 2 draws the picture, and the water looks its reflections up
// in that panorama instead of integrating the atmosphere again per pixel.
//   - sky: single-scattering Rayleigh + Mie atmosphere, sun disc, a drifting
//     cloud layer, moon and stars at night;
//   - sea: ray marched against the Gerstner height field, shaded with a
//     Fresnel mix of the reflected sky and the scattering water body, sun
//     glitter from a slope distribution whose width grows with the detail
//     that is filtered away in the distance, light through the wave crests,
//     and foam where the Jacobian says the surface folds over.

uniform vec2  uResolution;
uniform float uTime;
uniform vec3  uEye;
uniform vec3  uFwd;
uniform vec3  uRight;
uniform vec3  uUp;
uniform float uFov;            // tan(half vertical field of view)

uniform vec3  uSunDir;
uniform vec3  uSunColor;       // sun radiance reaching sea level (Scinc integrates it)
uniform vec3  uSkyAmbient;     // sky irradiance on a horizontal surface / pi
uniform vec3  uCloudSun;       // sun radiance reaching the cloud layer
uniform vec3  uMoonDir;
uniform float uMoonLight;      // 0 by day, up to 1 on a clear night
uniform float uExposure;
uniform float uCloudCover;     // 0 clear .. 1 overcast
uniform vec2  uCloudOffset;    // cloud drift in metres

#define NW 12
uniform vec4  uWA[NW];         // direction x, direction z, wavenumber k, amplitude A
uniform vec4  uWB[NW];         // 0, phase (time part accumulated by Scinc), Q*k*A, 0
uniform vec2  uWind;           // unit wind direction
uniform float uDetailL;        // longest detail wavelength (m)
uniform float uDetailS;        // slope amplitude k*A of each detail wave
uniform float uFoam;           // foam amount
uniform int   uPass;           // 1: sky panorama, 0: the picture
uniform sampler2D uSkyTex;     // the panorama from pass 1
uniform float uMaxAmp;         // sum of amplitudes: the slab the sea lives in
uniform float uChop;           // sum of Q*k*A: how close the crests come to folding

#define PI 3.14159265

// ------------------------------------------------------------ atmosphere ---
#define R_PLANET 6371e3
#define R_ATMOS  6471e3
const vec3  BETA_R = vec3(5.5e-6, 13.0e-6, 22.4e-6);
const float BETA_M = 7e-6;
const float H_R = 8000.0;
const float H_M = 1200.0;
const float G_M = 0.76;
const float SUN_I = 20.0;

// distance along a ray from inside the atmosphere to its outer shell
float AtmosExit(vec3 o, vec3 d)
{
	float b = dot(o, d);
	float c = dot(o, o) - R_ATMOS * R_ATMOS;
	return -b + sqrt(b * b - c);
}

// in-scattered sky radiance along view direction d, lit from direction s
vec3 Atmosphere(vec3 d, vec3 s)
{
	vec3 o = vec3(0.0, R_PLANET + 20.0, 0.0);
	float tmax = AtmosExit(o, d);
	vec3 sumR = vec3(0.0);
	vec3 sumM = vec3(0.0);
	float odR = 0.0;
	float odM = 0.0;
	// samples crowd towards the viewer (spacing grows quadratically): along
	// a grazing ray almost all the air is in the first few kilometres
	for (int i = 0; i < 10; i++)
	{
		float u0 = float(i) / 10.0;
		float u1 = float(i + 1) / 10.0;
		float ds = tmax * (u1 * u1 - u0 * u0);
		vec3 p = o + d * (tmax * 0.5 * (u0 * u0 + u1 * u1));
		float h = length(p) - R_PLANET;
		float hr = exp(-h / H_R) * ds;
		float hm = exp(-h / H_M) * ds;
		odR += hr;
		odM += hm;
		// optical depth towards the light; rays into the planet are in shadow
		float b = dot(p, s);
		float c = dot(p, p) - R_PLANET * R_PLANET;
		if (b < 0.0 && b * b - c > 0.0)
			continue;
		float tl = AtmosExit(p, s);
		float lR = 0.0;
		float lM = 0.0;
		for (int j = 0; j < 5; j++)
		{
			float v0 = float(j) / 5.0;
			float v1 = float(j + 1) / 5.0;
			float dl = tl * (v1 * v1 - v0 * v0);
			vec3 q = p + s * (tl * 0.5 * (v0 * v0 + v1 * v1));
			float hq = length(q) - R_PLANET;
			lR += exp(-hq / H_R) * dl;
			lM += exp(-hq / H_M) * dl;
		}
		vec3 att = exp(-(BETA_R * (odR + lR) + BETA_M * 1.1 * (odM + lM)));
		sumR += att * hr;
		sumM += att * hm;
	}
	float mu = dot(d, s);
	float pR = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
	float g2 = G_M * G_M;
	float pM = 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + mu * mu)) /
		((2.0 + g2) * pow(1.0 + g2 - 2.0 * mu * G_M, 1.5));
	return SUN_I * (pR * BETA_R * sumR + pM * BETA_M * sumM);
}

// ------------------------------------------------------------ noise -------
float Hash(vec2 p)
{
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float Noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(Hash(i), Hash(i + vec2(1.0, 0.0)), u.x),
	           mix(Hash(i + vec2(0.0, 1.0)), Hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float Fbm(vec2 p)
{
	float v = 0.0;
	float a = 0.5;
	for (int i = 0; i < 5; i++)
	{
		v += a * Noise(p);
		p = mat2(1.6, 1.2, -1.2, 1.6) * p;
		a *= 0.5;
	}
	return v;
}

// ------------------------------------------------------------ sky ---------
#define CLOUD_H 1500.0

// cloud layer seen along d (d.y > 0): rgb = light from the cloud, a = opacity
vec4 Clouds(vec3 d)
{
	if (d.y < 0.01 || uCloudCover <= 0.0)
		return vec4(0.0);
	float t = (CLOUD_H - uEye.y) / d.y;
	vec2 p = (uEye.xz + d.xz * t + uCloudOffset) * 0.00045;
	float n = Fbm(p);
	float cover = mix(0.72, 0.18, uCloudCover);
	float dens = smoothstep(cover, cover + 0.35, n);
	if (dens <= 0.0)
		return vec4(0.0);
	// self shadow: is there more cloud a little way towards the sun?
	vec2 toSun = normalize(uSunDir.xz + 1e-4) * 0.12;
	float n2 = Fbm(p + toSun);
	float lit = clamp(1.0 - 1.6 * (n2 - n) - 0.6 * dens, 0.15, 1.0);
	float mu = dot(d, uSunDir);
	float silver = 1.0 + 2.5 * pow(max(mu, 0.0), 12.0) * (1.0 - dens);
	vec3 col = uCloudSun * 0.11 * lit * silver
	         + uSkyAmbient * 1.1 + vec3(0.0009, 0.001, 0.0013) * uMoonLight * lit;
	col *= mix(1.0, 0.55, uCloudCover * dens);
	// thin out towards the horizon, where the layer is seen through haze
	float fade = smoothstep(0.01, 0.12, d.y);
	return vec4(col, dens * fade * 0.95);
}

// night: stars and moon (zero by day thanks to uMoonLight)
vec3 Night(vec3 d, bool primary)
{
	vec3 c = vec3(0.00008, 0.00014, 0.00032) * uMoonLight * (1.0 + 1.5 * (1.0 - d.y));
	float mu = dot(d, uMoonDir);
	c += vec3(0.0012, 0.0014, 0.0018) * uMoonLight * (pow(max(mu, 0.0), 40.0) + 0.3 * pow(max(mu, 0.0), 4.0));
	if (primary)
	{
		if (mu > 0.99996)
		{
			// a little surface texture on the disc
			vec3 r = d - uMoonDir * mu;
			float m = 0.75 + 0.25 * Noise(r.xy * 9000.0 + r.z * 4000.0);
			c += vec3(0.9, 0.92, 0.95) * m * 0.12 * uMoonLight;
		}
		// stars on a fixed grid in direction space
		vec2 sp = vec2(atan(d.z, d.x) * 180.0, d.y * 400.0);
		vec2 cell = floor(sp);
		float h = Hash(cell);
		if (h > 0.985)
		{
			vec2 off = vec2(Hash(cell + 7.1), Hash(cell + 3.7));
			float dd = length(fract(sp) - off);
			float tw = 0.7 + 0.3 * sin(uTime * (2.0 + 6.0 * h) + h * 50.0);
			c += vec3(0.006) * (h - 0.985) * 66.0 * tw * smoothstep(0.35, 0.0, dd)
			     * uMoonLight * smoothstep(0.0, 0.1, d.y);
		}
	}
	return c;
}

vec3 Sky(vec3 d, bool primary)
{
	vec3 dd = normalize(vec3(d.x, max(d.y, 0.0) + 0.002, d.z));
	vec3 c = Atmosphere(dd, uSunDir);
	c += Night(dd, primary);
	vec4 cl = Clouds(dd);
	c = mix(c, cl.rgb, cl.a);
	if (primary)
	{
		float mu = dot(d, uSunDir);
		float disc = smoothstep(0.99993, 0.99996, mu);
		c += uSunColor * 4.0 * disc * (1.0 - cl.a);
	}
	return c;
}

// ------------------------------------------------------------ panorama ----
// 256 x 96 texels: azimuth across, sqrt(elevation sine) up, so the rows
// crowd towards the horizon. Radiance is stored Reinhard-compressed after
// exposure, which keeps 8 bits enough from noon to a moonlit night.
#define PANO_W 256.0
#define PANO_H 96.0

vec3 PanoDir(vec2 frag)
{
	float az = (frag.x / PANO_W - 0.5) * 2.0 * PI;
	float v = (frag.y - 0.5) / (PANO_H - 1.0);
	float y = v * v;
	float r = sqrt(1.0 - y * y);
	return vec3(cos(az) * r, y, sin(az) * r);
}

vec3 SkyPano(vec3 d)
{
	float u = atan(d.z, d.x) / (2.0 * PI) + 0.5;
	float v = sqrt(clamp(d.y, 0.0, 1.0));
	vec3 e = texture2D(uSkyTex, vec2(u, (v * (PANO_H - 1.0) + 0.5) / PANO_H)).rgb;
	return e / max(1.0 - e, 1.0 / 255.0) / uExposure;
}

// ------------------------------------------------------------ waves -------
// Gerstner displacement of the undisplaced surface point x (horizontal
// shift in .xz, height in .y), summed over the first n waves.
vec3 Displace(vec2 x, int n)
{
	vec3 d = vec3(0.0);
	for (int i = 0; i < NW; i++)
	{
		if (i >= n)
			break;
		vec4 a = uWA[i];
		vec4 b = uWB[i];
		float th = a.z * dot(a.xy, x) + b.y;
		float c = cos(th);
		float q = b.z / a.z;         // Q * A: horizontal swing
		d.x += q * a.x * c;
		d.z += q * a.y * c;
		d.y += a.w * sin(th);
	}
	return d;
}

// surface height at world position xz: invert the horizontal shift with two
// fixed-point steps, then read the height of the point that lands there
float HeightAt(vec2 xz, int n)
{
	vec2 x = xz;
	int steps = n == NW ? 3 : 2;
	for (int k = 0; k < 3; k++)
	{
		if (k >= steps)
			break;
		x = xz - Displace(x, n).xz;
	}
	return Displace(x, n).y;
}

// the undisplaced point that lands on xz: x + D(x) = xz, solved by Newton
// steps, J = I - sum Q k A s D D^T is the same Jacobian that drives the foam
vec2 Undisplace(vec2 xz)
{
	vec2 x = xz - Displace(xz, NW).xz;
	for (int k = 0; k < 4; k++)
	{
		vec2 f = -xz;
		float jxx = 1.0;
		float jzz = 1.0;
		float jxz = 0.0;
		for (int i = 0; i < NW; i++)
		{
			vec4 a = uWA[i];
			vec4 b = uWB[i];
			float th = a.z * dot(a.xy, x) + b.y;
			float s = sin(th);
			f += (b.z / a.z * cos(th)) * a.xy;
			jxx -= b.z * a.x * a.x * s;
			jzz -= b.z * a.y * a.y * s;
			jxz -= b.z * a.x * a.y * s;
		}
		f += x;
		float det = max(jxx * jzz - jxz * jxz, 0.05);
		x -= vec2(jzz * f.x - jxz * f.y, jxx * f.y - jxz * f.x) / det;
	}
	return x;
}

// detail waves (normal only): a fan of shorter waves around the wind. Each
// one shorter than a few pixel footprints is faded out and its slope
// variance moved into the glitter roughness instead, so the far sea neither
// sparkles nor aliases.
vec3 Detail(vec2 x, float foot, out float slopeVar)
{
	vec2 g = vec2(0.0);
	slopeVar = 0.0;
	float L = uDetailL;
	float ang = atan(uWind.y, uWind.x);
	for (int i = 0; i < 20; i++)
	{
		float fi = float(i);
		float a = ang + (fract(sin(fi * 12.9898) * 43758.5453) - 0.5) * 2.4;
		vec2 dir = vec2(cos(a), sin(a));
		float k = 2.0 * PI / L;
		float w = sqrt(9.81 * k);
		float s = uDetailS;
		float vis = smoothstep(1.5, 5.0, L / foot);
		float th = k * dot(dir, x) - w * uTime + fi * 1.7;
		g += dir * (s * vis * cos(th));
		slopeVar += 0.5 * s * s * (1.0 - vis);
		L *= 0.8;
	}
	return vec3(-g.x, 1.0, -g.y);
}

// ------------------------------------------------------------ shading -----
vec3 Tonemap(vec3 c)
{
	c *= uExposure;
	c = (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);   // ACES fit
	return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

float FresnelW(float cosT)
{
	return 0.02 + 0.98 * pow(1.0 - cosT, 5.0);
}

void main()
{
	if (uPass == 1)
	{
		vec3 d = PanoDir(gl_FragCoord.xy);
		vec3 dd = normalize(vec3(d.x, d.y + 0.002, d.z));
		vec3 c = Atmosphere(dd, uSunDir) + Night(dd, false);
		vec4 cl = Clouds(dd);
		vec3 e = mix(c, cl.rgb, cl.a) * uExposure;
		gl_FragColor = vec4(e / (1.0 + e), 1.0);
		return;
	}

	vec2 uv = (2.0 * gl_FragCoord.xy - uResolution) / uResolution.y;
	vec3 rd = normalize(uFwd + uv.x * uFov * uRight + uv.y * uFov * uUp);
	float pixAngle = 2.0 * uFov / uResolution.y;

	vec3 col;
	// --- find the sea surface: march down from the top of the wave slab ---
	float top = uMaxAmp + 0.05;
	bool hit = false;
	float t = 0.0;
	if (rd.y < 0.0 || uEye.y < top)
	{
		if (uEye.y > top)
			t = (uEye.y - top) / -rd.y;
		float tFar = 6000.0;
		float tPrev = t;
		float fPrev = 1.0;
		for (int i = 0; i < 90; i++)
		{
			vec3 p = uEye + rd * t;
			int n = NW;
			if (t > 150.0) n = 8;
			if (t > 600.0) n = 5;
			if (t > 1500.0) n = 3;
			float f = p.y - HeightAt(p.xz, n);
			if (f < 0.0)
			{
				// refine between the last point above and this one below
				for (int j = 0; j < 5; j++)
				{
					float tm = mix(tPrev, t, fPrev / (fPrev - f));
					vec3 pm = uEye + rd * tm;
					float fm = pm.y - HeightAt(pm.xz, n);
					if (fm < 0.0) { t = tm; f = fm; }
					else { tPrev = tm; fPrev = fm; }
				}
				t = mix(tPrev, t, fPrev / (fPrev - f));
				hit = true;
				break;
			}
			if (f < 0.002 * t) { hit = true; break; }
			tPrev = t;
			fPrev = f;
			t += max(f * 0.7, 0.01 + t * 0.004);
			if (t > tFar || (rd.y >= 0.0 && p.y > top))
				break;
		}
		if (!hit && rd.y < 0.0)
		{
			// past the march range: the flat mean sea
			t = uEye.y / -rd.y;
			hit = true;
		}
	}

	if (!hit)
	{
		col = Sky(rd, true);
	}
	else
	{
		vec3 p = uEye + rd * t;
		vec2 x = Undisplace(p.xz);

		// pixel footprint on the water (grows at grazing angles)
		float foot = t * pixAngle / max(-rd.y, 0.02);

		// analytic Gerstner normal and Jacobian at the undisplaced point;
		// waves too short for the footprint fade out of the normal and their
		// slope variance goes to the glitter instead (as with the detail)
		float gVar = 0.0;
		float nx = 0.0;
		float nz = 0.0;
		float ny = 1.0;
		float jxx = 1.0;
		float jzz = 1.0;
		float jxz = 0.0;
		float hgt = 0.0;
		for (int i = 0; i < NW; i++)
		{
			vec4 a = uWA[i];
			vec4 b = uWB[i];
			float th = a.z * dot(a.xy, x) + b.y;
			float vis = smoothstep(1.5, 5.0, 2.0 * PI / (a.z * foot));
			float s = sin(th) * vis;
			float c = cos(th) * vis;
			float wa = a.z * a.w;
			gVar += 0.5 * wa * wa * (1.0 - vis);
			nx -= a.x * wa * c;
			nz -= a.y * wa * c;
			ny -= b.z * s;
			jxx -= b.z * a.x * a.x * s;
			jzz -= b.z * a.y * a.y * s;
			jxz -= b.z * a.x * a.y * s;
			hgt += a.w * sin(th);
		}
		vec3 nG = normalize(vec3(nx, ny, nz));
		float jac = jxx * jzz - jxz * jxz;

		float slopeVar;
		vec3 dn = Detail(p.xz, foot, slopeVar);
		// tilt the Gerstner normal by the detail slopes
		vec3 n = normalize(nG + vec3(dn.x, 0.0, dn.z));

		vec3 v = -rd;
		float nv = dot(n, v);
		if (nv < 0.02)
		{
			// a back-facing micro slope seen from a grazing angle: bend it
			n = normalize(n + v * (0.02 - nv));
			nv = 0.02;
		}

		// reflection of the sky
		vec3 r = reflect(rd, n);
		r.y = abs(r.y);
		float F = FresnelW(nv);
		// the filtered detail makes the surface rough at a distance: blur the
		// reflection there towards the cloud-averaged sky
		float rough = clamp((slopeVar + gVar) * 12.0, 0.0, 0.85);
		vec3 refl = SkyPano(r);
		vec3 rb = normalize(vec3(r.x, max(r.y, 0.0) + 0.05, r.z));
		vec3 skyAvg = mix(SkyPano(rb), uSkyAmbient * 1.3, uCloudCover * 0.6);
		refl = mix(refl, skyAvg, rough);

		// water body: light scattered back from below the surface
		vec3 deep = vec3(0.004, 0.016, 0.028);
		vec3 lightIn = uSunColor * max(uSunDir.y, 0.0) * 0.08 + uSkyAmbient * 1.4
		             + vec3(0.0001, 0.00015, 0.00025) * uMoonLight;
		vec3 body = deep * lightIn * 4.0;
		// light shining through the crests, towards the viewer
		vec3 hs = normalize(vec3(uSunDir.x, 0.0, uSunDir.z) + 1e-4);
		float back = pow(max(dot(-hs, rd * vec3(1.0, 0.0, 1.0)), 0.0), 3.0);
		float crest = clamp(hgt / max(uMaxAmp, 0.1) * 0.8 + 0.25, 0.0, 1.0);
		body += vec3(0.02, 0.12, 0.10) * uSunColor * crest * crest
		      * (0.25 + 0.9 * back) * max(uSunDir.y + 0.1, 0.0) * 0.6;

		col = mix(body, refl, F);

		// sun (and moon) glitter: Gaussian slope distribution whose variance
		// includes the detail lost to filtering at this distance
		float var = 0.0009 + slopeVar + gVar;
		vec3 L = uSunDir;
		vec3 h = normalize(L + v);
		float hy = max(dot(h, nG), 1e-3);
		vec3 hx = h - nG * dot(h, nG);
		float tan2 = dot(hx, hx) / (hy * hy);
		float nl = max(dot(nG, L), 0.0);
		if (L.y > -0.02)
		{
			float D = exp(-tan2 / (2.0 * var)) / (2.0 * PI * var * hy * hy * hy * hy);
			float spec = D * FresnelW(max(dot(h, v), 0.0)) / (4.0 * max(nv, 0.1));
			col += uSunColor * spec * 0.007 * smoothstep(-0.02, 0.02, L.y)
			     * (1.0 - 0.8 * uCloudCover);
		}
		if (uMoonLight > 0.0)
		{
			vec3 hm = normalize(uMoonDir + v);
			float my = max(dot(hm, nG), 1e-3);
			vec3 mx = hm - nG * dot(hm, nG);
			float mt2 = dot(mx, mx) / (my * my);
			float D = exp(-mt2 / (2.0 * var)) / (2.0 * PI * var * my * my * my * my);
			col += vec3(0.75, 0.8, 0.9) * D * FresnelW(max(dot(hm, v), 0.0))
			     / (4.0 * max(nv, 0.1)) * 0.0004 * uMoonLight * smoothstep(0.0, 0.05, uMoonDir.y);
		}

		// foam where the crests come close to folding over: fold = 1 - J,
		// relative to the most the wave set can fold; more wind, lower
		// threshold. Broken up by noise drawn out into streaks along the wind.
		float fold = (1.0 - jac) / max(uChop, 0.05);
		vec2 wp = vec2(dot(p.xz, uWind), dot(p.xz, vec2(-uWind.y, uWind.x)));
		float fn = Fbm(wp * vec2(0.12, 0.5) + vec2(uTime * 0.1, 0.0)) * 0.65
		         + Noise(p.xz * 1.7) * 0.35;
		// (random phases rarely line all the waves up: fold seldom passes 0.5)
		float thr = mix(0.62, 0.16, uFoam);
		float foam = smoothstep(thr, thr + 0.22, fold + 0.4 * (fn - 0.5));
		// old foam left behind on the slopes, in streaks
		foam += smoothstep(0.62, 0.78, fn) * smoothstep(thr - 0.3, thr, fold) * 0.5 * uFoam;
		foam *= smoothstep(0.0, 0.35, uFoam) * clamp(1.0 - foot * 0.2, 0.3, 1.0);
		vec3 foamCol = uSunColor * max(nl, 0.0) * 0.05 + uSkyAmbient * 2.2
		             + vec3(0.0006, 0.0007, 0.0009) * uMoonLight;
		col = mix(col, foamCol, clamp(foam, 0.0, 1.0) * 0.85);

		// aerial perspective: fade into the horizon sky
		vec3 hz = SkyPano(vec3(rd.x, 0.0, rd.z));
		float fog = 1.0 - exp(-t * 0.00012);
		col = mix(col, hz, fog);
	}

	gl_FragColor = vec4(Tonemap(col), 1.0);
}
