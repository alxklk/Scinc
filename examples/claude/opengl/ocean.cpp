// ocean.cpp - open sea and sky from dawn to night, ray marched per pixel by
// a GLSL fragment shader (ocean.frag) on a fullscreen quad.
//
// The work is split by what varies per pixel. Scinc does everything that is
// the same for the whole frame:
//   - builds the sea from the wind: 8 Gerstner waves whose peak wavelength
//     follows the Pierson-Moskowitz spectrum for the wind speed, fanned out
//     around the wind direction, with choppiness and foam rising with the
//     wind; each wave's phase is advanced by its deep-water frequency
//     w = sqrt(g k), so changing the wind never makes the waves jump;
//   - moves the sun and moon across the sky with the time of day and
//     integrates sunlight through the same Rayleigh + Mie atmosphere the
//     shader uses, giving the sun colour, the sky ambient and the exposure;
//   - in boat mode, evaluates the wave field under the camera so the view
//     rides the swell, heaving and rolling with it.
// The shader then draws the sky (atmosphere, sun, clouds, moon, stars) and
// the sea (reflection, glitter, light through the crests, foam). Each frame
// takes two passes: the shader first renders a small sky panorama into the
// corner of the back buffer, Scinc copies it into a texture, and the full
// picture then reads the water's reflections from it.
//
//   ./build_so.sh                    (once, builds scgl_dl.so)
//   Scinc ocean.cpp [width height]   (default 960x540)
//   nvrun Scinc ocean.cpp            (on the NVIDIA GPU)
//
// Controls:
//   mouse drag  look around          wheel     eye height (1.5 .. 120 m)
//   [ / ]       time of day -/+ 20 minutes
//   T           time lapse on/off    - / +     wind speed 2 .. 20 m/s
//   , / .       turn the wind        C         cloud cover (clear .. overcast)
//   B           boat mode (ride the waves)
//   space       pause                H         home (calm sunset)
//   F           fullscreen           S         save ocean.bmp
//   R           reload ocean.frag    Esc / Q   quit

#include <stdio.h>
#include "scgl.h"
#include "../../algorithm/Token.h"   // SToken / MakeTokens: parse the CLI resolution
#include "../../algorithm/Lex.h"

typedef char* pchar;

#define PI 3.14159265358979
#define GRAV 9.81
#define NW 12

// ---- wave set (uploaded as uWA / uWB, 4 floats per wave) ----
float wa[48];          // direction x, direction z, k, amplitude
float wb[48];          // 0, phase, Q*k*A, 0
float waveSpread[NW];  // fixed random layout, so a new wind keeps the sea's character
float waveLen[NW];
float wavePhase[NW];
float maxAmp = 1.0;
float chop = 0.5;
float detailL = 4.0;
float detailS = 0.04;
float foamAmt = 0.0;

float wind = 6.0;          // m/s, smoothed towards windTarget
float windTarget = 6.0;
float windAngle = 2.6;     // radians, direction the wind blows towards
float windX = 0.0;
float windZ = 0.0;

int rngState = 20261028;
float Rand()
{
	rngState = rngState * 1103515245 + 12345;
	return ((rngState >> 16) & 0x7fff) / 32768.0;
}

void InitWaveLayout()
{
	for (int i = 0; i < NW; i++)
	{
		// longer waves keep close to the wind, short ones spread out
		waveSpread[i] = (Rand() - 0.5) * (0.5 + 0.12 * i);
		waveLen[i] = pow(0.70, i) * (0.9 + 0.2 * Rand());
		wavePhase[i] = Rand() * 2.0 * PI;
	}
}

// rebuild wavenumbers, amplitudes and directions for the current wind
void BuildWaves()
{
	// Pierson-Moskowitz peak: w_p = 0.855 g / U, k_p = w_p^2 / g
	float wp = 0.855 * GRAV / wind;
	float kp = wp * wp / GRAV;
	float Lp = 2.0 * PI / kp;
	// beyond ~0.8 the inverse of the horizontal shift stops converging
	chop = 0.35 + 0.035 * wind;
	if (chop > 0.85) chop = 0.85;
	float wsum = 0.0;
	for (int i = 0; i < NW; i++)
		wsum = wsum + pow(0.85, i);
	maxAmp = 0.0;
	for (int i = 0; i < NW; i++)
	{
		float L = Lp * waveLen[i];
		float k = 2.0 * PI / L;
		float s = 0.055 + 0.002 * wind;         // steepness k*A
		float A = s / k;
		float a = windAngle + waveSpread[i];
		wa[i * 4 + 0] = cos(a);
		wa[i * 4 + 1] = sin(a);
		wa[i * 4 + 2] = k;
		wa[i * 4 + 3] = A;
		wb[i * 4 + 0] = 0.0;
		wb[i * 4 + 2] = chop * pow(0.85, i) / wsum;
		wb[i * 4 + 3] = 0.0;
		maxAmp = maxAmp + A;
	}
	detailL = Lp * pow(0.70, NW) * 1.3;
	// the 20 detail waves in the shader make up what the Gerstner waves
	// leave of the Cox-Munk mean square slope, 0.003 + 0.00512 U
	float gVar = 0.0;
	for (int i = 0; i < NW; i++)
		gVar = gVar + 0.5 * wa[i * 4 + 2] * wa[i * 4 + 3] * wa[i * 4 + 2] * wa[i * 4 + 3];
	float dVar = 0.003 + 0.00512 * wind - gVar;
	if (dVar < 0.002) dVar = 0.002;
	detailS = sqrt(2.0 * dVar / 20.0);
	foamAmt = (wind - 4.0) / 8.0;
	if (foamAmt < 0.0) foamAmt = 0.0;
	if (foamAmt > 1.0) foamAmt = 1.0;
	windX = cos(windAngle);
	windZ = sin(windAngle);
}

void AdvanceWaves(float dt)
{
	for (int i = 0; i < NW; i++)
	{
		float w = sqrt(GRAV * wa[i * 4 + 2]);
		wavePhase[i] = wavePhase[i] - w * dt;
		if (wavePhase[i] < -1000.0)
			wavePhase[i] = wavePhase[i] + 2.0 * PI * 160.0;
		wb[i * 4 + 1] = wavePhase[i];
	}
}

// Gerstner displacement of undisplaced point (x, z): out dx, dy, dz
void Displace(float x, float z, float& dx, float& dy, float& dz)
{
	dx = 0.0;
	dy = 0.0;
	dz = 0.0;
	for (int i = 0; i < NW; i++)
	{
		float th = wa[i * 4 + 2] * (wa[i * 4] * x + wa[i * 4 + 1] * z) + wb[i * 4 + 1];
		float q = wb[i * 4 + 2] / wa[i * 4 + 2];
		float c = cos(th);
		dx = dx + q * wa[i * 4] * c;
		dz = dz + q * wa[i * 4 + 1] * c;
		dy = dy + wa[i * 4 + 3] * sin(th);
	}
}

// sea height at world (x, z), as the shader computes it
float SeaHeight(float x, float z)
{
	float px = x;
	float pz = z;
	float dx;
	float dy;
	float dz;
	for (int k = 0; k < 6; k++)
	{
		Displace(px, pz, dx, dy, dz);
		px = x - dx;
		pz = z - dz;
	}
	Displace(px, pz, dx, dy, dz);
	return dy;
}

// ---- atmosphere (same model and constants as ocean.frag) ----
#define R_PLANET 6371000.0
#define R_ATMOS  6471000.0
#define E_NUM 2.718281828459045
#define BETA_M 7e-6

float Exp(float x)
{
	return pow(E_NUM, x);
}

float AtmosExit(float oy, float dx, float dy, float dz)
{
	// origin (0, oy, 0); |d| = 1
	float b = oy * dy;
	float c = oy * oy - R_ATMOS * R_ATMOS;
	return -b + sqrt(b * b - c);
}

// sun transmittance from altitude alt along direction (sx, sy, sz): out r, g, b
void SunTransmittance(float alt, float sx, float sy, float sz, float& r, float& g, float& b)
{
	float oy = R_PLANET + alt;
	float t = AtmosExit(oy, sx, sy, sz);
	float odR = 0.0;
	float odM = 0.0;
	for (int i = 0; i < 24; i++)
	{
		// quadratic spacing, dense near the ground (as in the shader)
		float u0 = i / 24.0;
		float u1 = (i + 1) / 24.0;
		float ds = t * (u1 * u1 - u0 * u0);
		float s = t * 0.5 * (u0 * u0 + u1 * u1);
		float px = sx * s;
		float py = oy + sy * s;
		float pz = sz * s;
		float h = sqrt(px * px + py * py + pz * pz) - R_PLANET;
		if (h < 0.0)
		{
			// the ray passes through the planet: the sun is set
			odR = 1e9;
			odM = 1e9;
			break;
		}
		odR = odR + Exp(-h / 8000.0) * ds;
		odM = odM + Exp(-h / 1200.0) * ds;
	}
	float m = BETA_M * 1.1 * odM;
	r = Exp(-(5.5e-6 * odR + m));
	g = Exp(-(13.0e-6 * odR + m));
	b = Exp(-(22.4e-6 * odR + m));
}

// sky radiance in direction d lit by s (as ocean.frag Atmosphere)
void SkyRadiance(float dx, float dy, float dz, float sx, float sy, float sz,
	float& r, float& g, float& b)
{
	float oy = R_PLANET + 20.0;
	float tmax = AtmosExit(oy, dx, dy, dz);
	float sRr = 0.0;
	float sRg = 0.0;
	float sRb = 0.0;
	float sMr = 0.0;
	float sMg = 0.0;
	float sMb = 0.0;
	float odR = 0.0;
	float odM = 0.0;
	for (int i = 0; i < 10; i++)
	{
		float u0 = i / 10.0;
		float u1 = (i + 1) / 10.0;
		float ds = tmax * (u1 * u1 - u0 * u0);
		float st = tmax * 0.5 * (u0 * u0 + u1 * u1);
		float px = dx * st;
		float py = oy + dy * st;
		float pz = dz * st;
		float h = sqrt(px * px + py * py + pz * pz) - R_PLANET;
		float hr = Exp(-h / 8000.0) * ds;
		float hm = Exp(-h / 1200.0) * ds;
		odR = odR + hr;
		odM = odM + hm;
		float bb = px * sx + py * sy + pz * sz;
		float cc = px * px + py * py + pz * pz - R_PLANET * R_PLANET;
		if (bb < 0.0 && bb * bb - cc > 0.0)
			continue;
		float b2 = bb;
		float c2 = px * px + py * py + pz * pz - R_ATMOS * R_ATMOS;
		float tl = -b2 + sqrt(b2 * b2 - c2);
		float lR = 0.0;
		float lM = 0.0;
		for (int j = 0; j < 5; j++)
		{
			float v0 = j / 5.0;
			float v1 = (j + 1) / 5.0;
			float dl = tl * (v1 * v1 - v0 * v0);
			float sl = tl * 0.5 * (v0 * v0 + v1 * v1);
			float qx = px + sx * sl;
			float qy = py + sy * sl;
			float qz = pz + sz * sl;
			float hq = sqrt(qx * qx + qy * qy + qz * qz) - R_PLANET;
			lR = lR + Exp(-hq / 8000.0) * dl;
			lM = lM + Exp(-hq / 1200.0) * dl;
		}
		float m = BETA_M * 1.1 * (odM + lM);
		float ar = Exp(-(5.5e-6 * (odR + lR) + m));
		float ag = Exp(-(13.0e-6 * (odR + lR) + m));
		float ab = Exp(-(22.4e-6 * (odR + lR) + m));
		sRr = sRr + ar * hr;
		sRg = sRg + ag * hr;
		sRb = sRb + ab * hr;
		sMr = sMr + ar * hm;
		sMg = sMg + ag * hm;
		sMb = sMb + ab * hm;
	}
	float mu = dx * sx + dy * sy + dz * sz;
	float pR = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
	float g2 = 0.76 * 0.76;
	float pM = 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + mu * mu)) /
		((2.0 + g2) * pow(1.0 + g2 - 2.0 * mu * 0.76, 1.5));
	r = 20.0 * (pR * 5.5e-6 * sRr + pM * BETA_M * sMr);
	g = 20.0 * (pR * 13.0e-6 * sRg + pM * BETA_M * sMg);
	b = 20.0 * (pR * 22.4e-6 * sRb + pM * BETA_M * sMb);
}

// ---- sun, moon and light for the current time of day ----
float hour = 17.85;
float sunX;
float sunY;
float sunZ;
float moonX;
float moonY;
float moonZ;
float sunR;
float sunG;
float sunB;
float cloudSunR;
float cloudSunG;
float cloudSunB;
float ambR;
float ambG;
float ambB;
float moonLight = 0.0;
float exposure = 1.0;
float cloudCover = 0.35;

void CelestialDir(float a, float& x, float& y, float& z)
{
	// a = 0 rises in the east (+x), pi sets in the west (-x); the path is
	// tilted 40 degrees towards the south (+z)
	float tilt = 0.70;
	x = cos(a);
	y = sin(a) * cos(tilt);
	z = sin(a) * sin(tilt);
}

float Smooth(float e0, float e1, float x)
{
	float t = (x - e0) / (e1 - e0);
	if (t < 0.0) t = 0.0;
	if (t > 1.0) t = 1.0;
	return t * t * (3.0 - 2.0 * t);
}

void UpdateLight()
{
	float a = (hour - 6.0) / 12.0 * PI;
	CelestialDir(a, sunX, sunY, sunZ);
	// a waxing gibbous moon trailing the sun by ~5.5 hours: high at sunset,
	// low in the west late in the evening
	CelestialDir(a - 1.45, moonX, moonY, moonZ);

	float tr;
	float tg;
	float tb;
	SunTransmittance(20.0, sunX, sunY, sunZ, tr, tg, tb);
	sunR = 20.0 * tr;
	sunG = 20.0 * tg;
	sunB = 20.0 * tb;
	// the clouds at 1500 m keep the sun a while after it has set for the sea
	SunTransmittance(1500.0, sunX, sunY, sunZ, tr, tg, tb);
	cloudSunR = 20.0 * tr;
	cloudSunG = 20.0 * tg;
	cloudSunB = 20.0 * tb;

	// sky ambient: zenith plus four directions at 25 degrees elevation
	ambR = 0.0;
	ambG = 0.0;
	ambB = 0.0;
	for (int i = 0; i < 5; i++)
	{
		float dx = 0.0;
		float dy = 1.0;
		float dz = 0.0;
		if (i > 0)
		{
			float az = (i - 1) * PI * 0.5 + 0.4;
			dx = cos(az) * 0.906;
			dy = 0.423;
			dz = sin(az) * 0.906;
		}
		float r;
		float g;
		float b;
		SkyRadiance(dx, dy, dz, sunX, sunY, sunZ, r, g, b);
		ambR = ambR + r * 0.2;
		ambG = ambG + g * 0.2;
		ambB = ambB + b * 0.2;
	}
	// clouds grey the light: less direct sun, a whiter ambient
	float cv = cloudCover * 0.8;
	float grey = (ambR + ambG + ambB) / 3.0;
	float dim = 1.0 - 0.45 * cloudCover * cloudCover;
	ambR = (ambR + (grey - ambR) * cv * 0.6) * dim;
	ambG = (ambG + (grey - ambG) * cv * 0.6) * dim;
	ambB = (ambB + (grey - ambB) * cv * 0.6) * dim;

	moonLight = Smooth(0.02, -0.12, sunY) * Smooth(-0.05, 0.15, moonY);

	// exposure: fixed while there is daylight, like a photographer keeping
	// the sunset's colours; opened up only as the sky darkens into night
	float lum = 0.2126 * ambR + 0.7152 * ambG + 0.0722 * ambB;
	exposure = 0.45;
	if (lum < 0.15)
		exposure = 0.45 * 0.15 / (lum + 0.0011);
	if (exposure > 60.0) exposure = 60.0;
}

// ---- camera ----
#define HOME_YAW   3.25
#define HOME_PITCH -0.03
#define HOME_EYE   4.0

float yaw = HOME_YAW;
float pitch = HOME_PITCH;
float eyeH = HOME_EYE;
float camY = HOME_EYE;
float roll = 0.0;
float bobPitch = 0.0;
int boat = 0;

// ---- shader ----
int prog = 0;
int uResolution;
int uTime;
int uEye;
int uFwd;
int uRight;
int uUp;
int uFov;
int uSunDir;
int uSunColor;
int uSkyAmbient;
int uCloudSun;
int uMoonDir;
int uMoonLight;
int uExposure;
int uCloudCover;
int uCloudOffset;
int uWA;
int uWB;
int uWind;
int uDetailL;
int uDetailS;
int uFoam;
int uMaxAmp;
int uPass;
int uSkyTex;
int uChop;

int LoadShaders()
{
	int p = glwLoadProgram("fullscreen.vert", "ocean.frag");
	if (p == 0)
		return 0;
	if (prog != 0)
		glDeleteProgram(prog);
	prog = p;
	uResolution  = glGetUniformLocation(prog, "uResolution");
	uTime        = glGetUniformLocation(prog, "uTime");
	uEye         = glGetUniformLocation(prog, "uEye");
	uFwd         = glGetUniformLocation(prog, "uFwd");
	uRight       = glGetUniformLocation(prog, "uRight");
	uUp          = glGetUniformLocation(prog, "uUp");
	uFov         = glGetUniformLocation(prog, "uFov");
	uSunDir      = glGetUniformLocation(prog, "uSunDir");
	uSunColor    = glGetUniformLocation(prog, "uSunColor");
	uSkyAmbient  = glGetUniformLocation(prog, "uSkyAmbient");
	uCloudSun    = glGetUniformLocation(prog, "uCloudSun");
	uMoonDir     = glGetUniformLocation(prog, "uMoonDir");
	uMoonLight   = glGetUniformLocation(prog, "uMoonLight");
	uExposure    = glGetUniformLocation(prog, "uExposure");
	uCloudCover  = glGetUniformLocation(prog, "uCloudCover");
	uCloudOffset = glGetUniformLocation(prog, "uCloudOffset");
	uWA          = glGetUniformLocation(prog, "uWA");
	uWB          = glGetUniformLocation(prog, "uWB");
	uWind        = glGetUniformLocation(prog, "uWind");
	uDetailL     = glGetUniformLocation(prog, "uDetailL");
	uDetailS     = glGetUniformLocation(prog, "uDetailS");
	uFoam        = glGetUniformLocation(prog, "uFoam");
	uMaxAmp      = glGetUniformLocation(prog, "uMaxAmp");
	uPass        = glGetUniformLocation(prog, "uPass");
	uSkyTex      = glGetUniformLocation(prog, "uSkyTex");
	uChop        = glGetUniformLocation(prog, "uChop");
	return 1;
}

#define PANO_W 256
#define PANO_H 96
int skyTex = 0;

void DrawFullscreenQuad()
{
	glBegin(GL_QUADS);
	glVertex2f(-1.0, -1.0);
	glVertex2f( 1.0, -1.0);
	glVertex2f( 1.0,  1.0);
	glVertex2f(-1.0,  1.0);
	glEnd();
}

float cloudX = 0.0;
float cloudZ = 0.0;

void SetUniforms(float simTime)
{
	glUniform2f(uResolution, glwWidth(), glwHeight());
	glUniform1f(uTime, simTime);

	// camera basis: yaw/pitch, then the boat's roll about the view axis
	float cp = cos(pitch + bobPitch);
	float fx = cp * cos(yaw);
	float fy = sin(pitch + bobPitch);
	float fz = cp * sin(yaw);
	// right = fwd x up(0,1,0), normalised
	float rx = -fz;
	float rz = fx;
	float rl = sqrt(rx * rx + rz * rz);
	rx = rx / rl;
	rz = rz / rl;
	float ry = 0.0;
	// up = right x fwd
	float ux = ry * fz - rz * fy;
	float uy = rz * fx - rx * fz;
	float uz = rx * fy - ry * fx;
	float cr = cos(roll);
	float sr = sin(roll);
	float rx2 = rx * cr + ux * sr;
	float ry2 = ry * cr + uy * sr;
	float rz2 = rz * cr + uz * sr;
	float ux2 = ux * cr - rx * sr;
	float uy2 = uy * cr - ry * sr;
	float uz2 = uz * cr - rz * sr;

	glUniform3f(uEye, 0.0, camY, 0.0);
	glUniform3f(uFwd, fx, fy, fz);
	glUniform3f(uRight, rx2, ry2, rz2);
	glUniform3f(uUp, ux2, uy2, uz2);
	glUniform1f(uFov, 0.42);

	glUniform3f(uSunDir, sunX, sunY, sunZ);
	glUniform3f(uSunColor, sunR, sunG, sunB);
	glUniform3f(uSkyAmbient, ambR, ambG, ambB);
	glUniform3f(uCloudSun, cloudSunR, cloudSunG, cloudSunB);
	glUniform3f(uMoonDir, moonX, moonY, moonZ);
	glUniform1f(uMoonLight, moonLight);
	glUniform1f(uExposure, exposure);
	glUniform1f(uCloudCover, cloudCover);
	glUniform2f(uCloudOffset, cloudX, cloudZ);

	glUniform4fv(uWA, NW, wa);
	glUniform4fv(uWB, NW, wb);
	glUniform2f(uWind, windX, windZ);
	glUniform1f(uDetailL, detailL);
	glUniform1f(uDetailS, detailS);
	glUniform1f(uFoam, foamAmt);
	glUniform1f(uMaxAmp, maxAmp);
	glUniform1i(uSkyTex, 0);
	glUniform1f(uChop, chop);
}

void Home()
{
	yaw = HOME_YAW;
	pitch = HOME_PITCH;
	eyeH = HOME_EYE;
	hour = 17.85;
	windTarget = 6.0;
	windAngle = 2.6;
	cloudCover = 0.35;
	boat = 0;
	roll = 0.0;
	bobPitch = 0.0;
}

int main(int argc, pchar* argv)
{
	int W = 960;
	int H = 540;
	if (argc >= 3)
	{
		SToken tw[16];
		SToken th[16];
		MakeTokens(tw, argv[1]);
		MakeTokens(th, argv[2]);
		if (tw[0].IsNumber()) W = (int)tw[0].GetNumber();
		if (th[0].IsNumber()) H = (int)th[0].GetNumber();
	}
	if (W < 16) W = 16;
	if (H < 16) H = 16;

	if (!glwInit(W, H, "Scinc ocean"))
	{
		printf("Cannot open OpenGL window\n");
		return 1;
	}
	char info[256];
	glwGetString(GL_RENDERER, info, 256);
	printf("GL_RENDERER = %s\n", info);
	fflush(stdout);

	if (!LoadShaders())
		return 1;

	// the sky panorama texture (filled by pass 1 every frame)
	skyTex = glwGenTexture();
	glBindTexture(GL_TEXTURE_2D, skyTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, PANO_W, PANO_H, 0);

	InitWaveLayout();
	BuildWaves();
	AdvanceWaves(0.0);

	int moving = 1;
	int timeLapse = 0;
	int fullscreen = 0;
	int saveNext = 0;
	int dragging = 0;
	int lastX = 0;
	int lastY = 0;
	int running = 1;
	float simTime = 0.0;
	float lightHour = -100.0;
	float lightCloud = -1.0;

	int frames = 0;
	double tPrev = glwTime();
	double tRate0 = tPrev;
	float fps = 0.0;

	while (running && glwPoll())
	{
		double t = glwTime();
		float dt = t - tPrev;
		tPrev = t;
		if (dt > 0.1) dt = 0.1;

		int key = glwGetKey();
		while (key != 0)
		{
			if (key == GLW_KEY_ESC || key == 'q' || key == 'Q')
				running = 0;
			else if (key == ' ')
				moving = 1 - moving;
			else if (key == '[')
				hour = hour - 1.0 / 3.0;
			else if (key == ']')
				hour = hour + 1.0 / 3.0;
			else if (key == 't' || key == 'T')
				timeLapse = 1 - timeLapse;
			else if (key == '-')
			{
				windTarget = windTarget - 2.0;
				if (windTarget < 2.0) windTarget = 2.0;
			}
			else if (key == '+' || key == '=')
			{
				windTarget = windTarget + 2.0;
				if (windTarget > 20.0) windTarget = 20.0;
			}
			else if (key == ',')
				windAngle = windAngle - 0.2;
			else if (key == '.')
				windAngle = windAngle + 0.2;
			else if (key == 'c' || key == 'C')
			{
				if (cloudCover < 0.1) cloudCover = 0.35;
				else if (cloudCover < 0.5) cloudCover = 0.65;
				else if (cloudCover < 0.8) cloudCover = 0.92;
				else cloudCover = 0.0;
			}
			else if (key == 'b' || key == 'B')
			{
				boat = 1 - boat;
				if (!boat)
				{
					roll = 0.0;
					bobPitch = 0.0;
				}
			}
			else if (key == 'f' || key == 'F')
			{
				fullscreen = 1 - fullscreen;
				glwSetFullscreen(fullscreen);
			}
			else if (key == 's' || key == 'S')
				saveNext = 1;
			else if (key == 'r' || key == 'R')
			{
				if (LoadShaders())
					printf("shaders reloaded\n");
				fflush(stdout);
			}
			else if (key == 'h' || key == 'H')
				Home();
			key = glwGetKey();
		}

		// mouse: drag to look around, wheel for eye height
		int mx = glwMouseX();
		int my = glwMouseY();
		if (glwMouseButtons() & 1)
		{
			if (dragging)
			{
				yaw = yaw + 0.004 * (mx - lastX);
				pitch = pitch - 0.004 * (my - lastY);
				if (pitch < -1.2) pitch = -1.2;
				if (pitch > 1.2) pitch = 1.2;
			}
			dragging = 1;
		}
		else
			dragging = 0;
		lastX = mx;
		lastY = my;
		int wheel = glwMouseWheel();
		if (wheel != 0)
		{
			eyeH = eyeH * pow(1.15, wheel);
			if (eyeH < 1.5) eyeH = 1.5;
			if (eyeH > 120.0) eyeH = 120.0;
		}

		if (hour < 0.0) hour = hour + 24.0;
		if (hour >= 24.0) hour = hour - 24.0;

		// the wind eases towards its target; the sea rebuilds as it does
		if (wind != windTarget || windX != cos(windAngle))
		{
			float dw = windTarget - wind;
			float stepW = 1.5 * dt;
			if (dw > stepW) dw = stepW;
			if (dw < -stepW) dw = -stepW;
			wind = wind + dw;
			BuildWaves();
		}

		if (moving)
		{
			simTime = simTime + dt;
			AdvanceWaves(dt);
			cloudX = cloudX - windX * wind * 1.5 * dt;
			cloudZ = cloudZ - windZ * wind * 1.5 * dt;
			if (timeLapse)
				hour = hour + dt * 0.25;       // an hour every 4 seconds
		}

		// the light is integrated through the atmosphere only when it changes
		if (hour != lightHour || cloudCover != lightCloud)
		{
			UpdateLight();
			lightHour = hour;
			lightCloud = cloudCover;
		}

		// camera height: keep clear of the crests; in boat mode ride them
		float h0 = SeaHeight(0.0, 0.0);
		if (boat)
		{
			// sample the swell around the hull for pitch and roll
			float e = 2.5;
			float fxh = cos(yaw);
			float fzh = sin(yaw);
			float hf = SeaHeight(fxh * e, fzh * e);
			float hb = SeaHeight(-fxh * e, -fzh * e);
			float hr = SeaHeight(-fzh * e, fxh * e);
			float hl = SeaHeight(fzh * e, -fxh * e);
			float k = dt * 3.0;
			if (k > 1.0) k = 1.0;
			float target = 1.5 + (h0 + (hf + hb + hr + hl) * 0.25) * 0.5;
			camY = camY + (target - camY) * k;
			bobPitch = bobPitch + (0.5 * (hf - hb) / (2.0 * e) - bobPitch) * k;
			roll = roll + (0.5 * (hr - hl) / (2.0 * e) - roll) * k;
		}
		else
		{
			camY = eyeH;
			if (camY < h0 + 1.2)
				camY = h0 + 1.2;
		}

		glUseProgram(prog);
		SetUniforms(simTime);

		// pass 1: the sky panorama in the corner of the back buffer, copied
		// to the texture the water reflects
		glViewport(0, 0, PANO_W, PANO_H);
		glUniform1i(uPass, 1);
		DrawFullscreenQuad();
		glBindTexture(GL_TEXTURE_2D, skyTex);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, PANO_W, PANO_H);

		// pass 2: the picture, over the whole window
		glViewport(0, 0, glwWidth(), glwHeight());
		glUniform1i(uPass, 0);
		DrawFullscreenQuad();

		if (saveNext)
		{
			if (glwSaveBMP("ocean.bmp"))
				printf("wrote ocean.bmp\n");
			fflush(stdout);
			saveNext = 0;
		}
		glwSwap();

		frames++;
		if (t - tRate0 >= 1.0)
		{
			fps = frames / (t - tRate0);
			int hh = (int)hour;
			int mm = (int)((hour - hh) * 60.0);
			char title[128];
			snprintf(title, 128, "Scinc ocean - %02d:%02d - wind %.0f m/s - %dx%d - %.1f FPS",
				hh, mm, wind, glwWidth(), glwHeight(), fps);
			glwSetTitle(title);
			tRate0 = t;
			frames = 0;
		}
	}

	glDeleteProgram(prog);
	glwDone();
	return 0;
}
