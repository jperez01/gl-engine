layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec4 WorldPos[];
in vec3 Normal[];
in vec2 TexCoords[];

out vec3 FragPos;
out vec3 WorldVoxelPos;
out vec3 FragNormal;
out vec2 FragTexCoords;

uniform int gridSize;
uniform mat4 projection;

void main() {
    vec3 v1 = WorldPos[1].xyz - WorldPos[0].xyz;
    vec3 v2 = WorldPos[2].xyz - WorldPos[0].xyz;
    vec3 p = abs(normalize(cross(v1, v2)));

    float prominentAxis = max(p.x, max(p.y, p.z));

    for (int i = 0; i < 3; i++) {
        FragNormal = Normal[i];
        FragTexCoords = TexCoords[i];
        FragPos = WorldPos[i].xyz;

        vec4 tempPos = projection * WorldPos[i];

        vec4 voxelPos = ((tempPos * 0.5 + 0.5) * gridSize);
        WorldVoxelPos = voxelPos.xyz;
        if (prominentAxis == p.x) {
            tempPos = vec4(tempPos.z, tempPos.y, tempPos.x, tempPos.w);
        } else if (prominentAxis == p.y) {
            tempPos = vec4(tempPos.x, tempPos.z, tempPos.y, tempPos.w);
        } else {
            tempPos = vec4(tempPos.x, tempPos.y, tempPos.z, tempPos.w);
        }

        gl_Position = tempPos;
        EmitVertex();
    }
    EndPrimitive();
}