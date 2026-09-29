#version 120
// Pass-through for a fullscreen quad: the host draws a quad with corners at
// (-1,-1)..(1,1) directly in clip space, so no matrices are involved.
void main()
{
	gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
}
