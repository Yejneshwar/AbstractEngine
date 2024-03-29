// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.025;
float gridMajorSize = 0.000000005;
float gridMinorSize = 10.0;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur. 
const float gridMinPixelsBetweenCells = 2.0;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
	);

const vec3 pos2D[4] = vec3[4](
	vec3(-1.0, -1.0, 0.0),
	vec3( 1.0, -1.0, 0.0),
	vec3( 1.0, 1.0,  0.0),
	vec3(-1.0, 1.0,  0.0)
	);

const vec2 tex[4] = vec2[4](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
	);

const int indices[6] = int[6](
	0, 1, 2, 2, 3, 0
	);