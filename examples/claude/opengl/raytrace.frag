#version 120
// GLSL port of ../graphic/raytrace.cpp: a metal still life on a checkered
// plane, ray traced per pixel on the GPU.
//   - sphere: silver mirror, resting on the ground
//   - cube:   bronze mirror, balanced on one vertex (space diagonal vertical)
//   - torus:  gold mirror, lying flat, sphere-traced through its SDF
//   - plane:  y = 0, checkerboard, reflectance 0.3, shadows from the light
//   - sky:    vertical gradient with a soft sun glow
// Metals are coloured mirrors: reflection carries a per-channel tint that
// each surface multiplies. Geometry and camera come from the host as
// uniforms (see raytrace_gl.cpp), so the scene can move.

uniform vec2  uResolution;
uniform vec3  uEye;
uniform vec3  uTarget;
uniform float uFov;
uniform int   uAA;         // AA*AA samples per pixel, 1..4

uniform vec4  uSphere;     // xyz center, w radius
uniform vec3  uCubeB;      // cube bottom vertex
uniform float uCubeL;      // cube edge length
uniform vec3  uCF0;        // cube edge axes (orthonormal tripod)
uniform vec3  uCF1;
uniform vec3  uCF2;
uniform vec3  uTorusC;     // torus center
uniform vec2  uTorusR;     // x major radius, y minor radius

#define MAXB        5
#define TORUS_STEPS 128

const vec3  LDIR       = vec3(0.4728662, 0.7881104, -0.3940552); // normalize(0.6, 1, -0.5)
const float PLANE_REFL = 0.3;
const vec3  SILVER     = vec3(0.95, 0.94, 0.90);
const vec3  GOLD       = vec3(1.00, 0.77, 0.34);
const vec3  BRONZE     = vec3(0.80, 0.50, 0.28);

struct Hit { int obj; float t; vec3 n; };   // obj: 0 miss,1 plane,2 sphere,3 cube,4 torus

vec3 cubeAxis(int k)
{
	if(k == 0) return uCF0;
	if(k == 1) return uCF1;
	return uCF2;
}

vec3 metalColor(int obj)
{
	if(obj == 2) return SILVER;
	if(obj == 4) return GOLD;
	return BRONZE;
}

vec3 skyColor(vec3 d)
{
	float t = 0.5 * (d.y + 1.0);
	vec3 base = mix(vec3(0.85, 0.90, 1.00), vec3(0.25, 0.45, 0.85), t);
	float sun = pow(max(0.0, dot(d, LDIR)), 250.0);
	return base + vec3(1.0, 0.95, 0.8) * sun;
}

bool hitSphereAny(vec3 o, vec3 d)
{
	vec3 oc = o - uSphere.xyz;
	float b = dot(oc, d);
	float c = dot(oc, oc) - uSphere.w * uSphere.w;
	float disc = b * b - c;
	return disc > 0.0 && -b - sqrt(disc) > 1e-3;
}

// one slab of the oriented box; returns false when the ray misses it
bool slab(float lo, float ld, int k, inout float tmin, inout float tmax, inout int axis, inout float sgn)
{
	if(abs(ld) < 1e-9)
		return lo >= 0.0 && lo <= uCubeL;
	float inv = 1.0 / ld;
	float ta = -lo * inv;
	float tb = (uCubeL - lo) * inv;
	float s = -1.0;
	if(ta > tb) { float tmp = ta; ta = tb; tb = tmp; s = 1.0; }
	if(ta > tmin) { tmin = ta; axis = k; sgn = s; }
	if(tb < tmax) tmax = tb;
	return true;
}

Hit hitCube(vec3 o, vec3 d)
{
	Hit h = Hit(0, 1e30, vec3(0.0, 1.0, 0.0));
	vec3 rel = o - uCubeB;
	float tmin = -1e30;
	float tmax = 1e30;
	int axis = 0;
	float sgn = -1.0;
	if(!slab(dot(rel, uCF0), dot(d, uCF0), 0, tmin, tmax, axis, sgn)) return h;
	if(!slab(dot(rel, uCF1), dot(d, uCF1), 1, tmin, tmax, axis, sgn)) return h;
	if(!slab(dot(rel, uCF2), dot(d, uCF2), 2, tmin, tmax, axis, sgn)) return h;
	if(tmin > tmax || tmax < 1e-3) return h;
	float t = tmin;
	if(t < 1e-3) t = tmax;
	if(t < 1e-3) return h;

	vec3 n = cubeAxis(axis) * sgn;
	if(dot(n, d) > 0.0) n = -n;
	return Hit(3, t, n);
}

float torusSDF(vec3 p)                  // p in torus-local coords, axis = y
{
	float a = length(p.xz) - uTorusR.x;
	return sqrt(a * a + p.y * p.y) - uTorusR.y;
}

Hit hitTorus(vec3 o, vec3 d)
{
	Hit h = Hit(0, 1e30, vec3(0.0, 1.0, 0.0));
	vec3 oc = o - uTorusC;
	float Rb = uTorusR.x + uTorusR.y + 1e-2;   // bounding sphere
	float b = dot(oc, d);
	float c = dot(oc, oc) - Rb * Rb;
	float disc = b * b - c;
	if(disc <= 0.0) return h;
	float sq = sqrt(disc);
	float t0 = -b - sq;
	float t1 = -b + sq;
	if(t1 < 1e-3) return h;
	float t = max(t0, 1e-3);

	for(int i = 0; i < TORUS_STEPS; i++)
	{
		vec3 p = o + d * t - uTorusC;
		float dist = torusSDF(p);
		if(dist < 1e-4)
		{
			float a = length(p.xz);
			float f = 1.0 - uTorusR.x / a;
			vec3 n = normalize(vec3(p.x * f, p.y, p.z * f));
			if(dot(n, d) > 0.0) n = -n;
			return Hit(4, t, n);
		}
		t += dist;
		if(t > t1) return h;
	}
	return h;
}

bool inShadow(vec3 p)
{
	vec3 so = p + vec3(0.0, 1e-3, 0.0);
	return hitSphereAny(so, LDIR) || hitCube(so, LDIR).obj != 0 || hitTorus(so, LDIR).obj != 0;
}

vec3 planeColor(vec3 p, vec3 d, float dist)
{
	float parity = mod(floor(p.x) + floor(p.z), 2.0);
	vec3 base = parity > 0.5 ? vec3(0.95) : vec3(0.08, 0.09, 0.12);

	float diff = max(0.0, LDIR.y);
	float sh = inShadow(p) ? 0.30 : 1.0;
	base *= 0.25 + 0.75 * diff * sh;

	float f = 1.0 / (1.0 + dist * dist * 0.0015);   // distance haze into the sky
	return mix(skyColor(d), base, f);
}

Hit intersect(vec3 o, vec3 d)
{
	Hit best = Hit(0, 1e30, vec3(0.0, 1.0, 0.0));

	if(abs(d.y) > 1e-6)
	{
		float t = -o.y / d.y;
		if(t > 1e-3 && t < best.t) best = Hit(1, t, vec3(0.0, 1.0, 0.0));
	}

	vec3 oc = o - uSphere.xyz;
	float b = dot(oc, d);
	float c = dot(oc, oc) - uSphere.w * uSphere.w;
	float disc = b * b - c;
	if(disc > 0.0)
	{
		float t = -b - sqrt(disc);
		if(t <= 1e-3) t = -b + sqrt(disc);
		if(t > 1e-3 && t < best.t)
			best = Hit(2, t, normalize(o + d * t - uSphere.xyz));
	}

	Hit hc = hitCube(o, d);
	if(hc.obj != 0 && hc.t < best.t) best = hc;

	Hit ht = hitTorus(o, d);
	if(ht.obj != 0 && ht.t < best.t) best = ht;

	return best;
}

vec3 trace(vec3 o, vec3 d)
{
	vec3 col = vec3(0.0);
	vec3 tint = vec3(1.0);              // per-channel reflected-light throughput
	for(int bounce = 0; bounce < MAXB; bounce++)
	{
		Hit h = intersect(o, d);
		if(h.obj == 0) { col += tint * skyColor(d); break; }

		vec3 hit = o + d * h.t;
		vec3 n = h.n;
		if(h.obj == 1)
		{
			col += tint * planeColor(hit, d, h.t) * (1.0 - PLANE_REFL);
			tint *= PLANE_REFL;                              // neutral mirror
		}
		else
		{
			vec3 mcol = metalColor(h.obj);
			vec3 hlf = normalize(LDIR - d);
			float spec = pow(max(0.0, dot(n, hlf)), 60.0);
			col += tint * mcol * spec;                       // tinted highlight
			tint *= mcol;                                    // tinted reflection
		}
		o = hit + n * 1e-3;
		d = reflect(d, n);
		if(max(tint.r, max(tint.g, tint.b)) < 0.01) break;
	}
	return col;
}

void main()
{
	vec3 fwd = normalize(uTarget - uEye);
	vec3 rgt = normalize(cross(fwd, vec3(0.0, 1.0, 0.0)));
	vec3 up  = cross(rgt, fwd);
	float aspect = uResolution.x / uResolution.y;
	vec2 pix = floor(gl_FragCoord.xy);    // gl_FragCoord y grows upward

	vec3 acc = vec3(0.0);
	for(int jy = 0; jy < 4; jy++)
	{
		if(jy >= uAA) break;
		for(int jx = 0; jx < 4; jx++)
		{
			if(jx >= uAA) break;
			vec2 p = pix + (vec2(jx, jy) + 0.5) / float(uAA);
			float sx = (2.0 * p.x / uResolution.x - 1.0) * aspect * uFov;
			float sy = (2.0 * p.y / uResolution.y - 1.0) * uFov;
			acc += trace(uEye, normalize(fwd + rgt * sx + up * sy));
		}
	}
	gl_FragColor = vec4(clamp(acc / float(uAA * uAA), 0.0, 1.0), 1.0);
}
