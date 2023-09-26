uniform sampler2D ourTexture;

vec4 chars(vec2 p, int c) 
{
     if (p.x<.0|| p.x>1. || p.y<0.|| p.y>1.) return vec4(0,0,0,1e5);
    //p = clamp(p, vec2(0.0), vec2(1.0)); // Clamp p to [0, 1]
	//p.y = -p.y + 1; // because of layout(origin_upper_left) in vec4 gl_FragCoord;
	return textureGrad( ourTexture, p/16. + fract( vec2(c, 15-c/16) / 16. ), dFdx(p/16.),dFdy(p/16.) );
}

// --- display int4
#if 0
vec4 pInt(vec2 p, float n) {  // webGL2 variant with dynamic size
    vec4 v = vec4(0);
    for (int i = int(n); i>0; i/=10, p.x += .5 )
        v += chars(p, 48+ i%10 );
    return v;
}
#else
vec4 pInt(vec2 p, float n) {
    vec4 v = vec4(0);
    if (n < 0.) 
        v += chars(p - vec2(-.5,0), 45 ),
        n = -n;

    for (float i = 3.; i>=0.; i--) 
        n /=  9.999999, // 10., // for windows :-(
        v += chars(p - .5*vec2(i,0), 48+ int(fract(n)*10.) );
    return v;
}
#endif

vec4 pFloat(vec2 p, float n) {
    vec4 v = vec4(0);
    if (n < 0.) v += chars(p - vec2(-.5,0), 45 ), n = -n;
    float upper = floor(n);
    float lower = fract(n)*1e4 + .5;  // mla fix for rounding lost decimals
    if (lower >= 1e4) { lower -= 1e4; upper++; }
    v += pInt(p,upper); p.x -= 2.;
    v += chars(p, 46);   p.x -= .5;
    v += pInt(p,lower);
    return v;
}

vec4 pFloat(vec2 p, vec2 vec) {
    float n = vec.x;
    vec4 v = vec4(0);
    if (n < 0.) v += chars(p - vec2(-.5,0), 45 ), n = -n;
    float upper = floor(n);
    float lower = fract(n)*1e4 + .5;  // mla fix for rounding lost decimals
    if (lower >= 1e4) { lower -= 1e4; upper++; }
    v += pInt(p,upper); p.x -= 2.;
    v += chars(p, 46);   p.x -= .5;
    v += pInt(p,lower);

    n = vec.y;
    p.x -= 2.6;
    if (n < 0.) v += chars(p - vec2(-.5,0), 45 ), n = -n;
    upper = floor(n);
    lower = fract(n)*1e4 + .5;  // mla fix for rounding lost decimals
    if (lower >= 1e4) { lower -= 1e4; upper++; }
    v += pInt(p,upper); p.x -= 2.;
    v += chars(p, 46);   p.x -= .5;
    v += pInt(p,lower);
    return v;
}

int CAPS=0;
#define low CAPS=32;
#define caps CAPS=0;
#define spc  U.x-=.5;
#define C(c) spc out_FragColor+= chars(U,64+CAPS+c);