#version 430 core

out vec4 FragColor;

in vec2 TexCoords;
in vec4 Offsets[3];
in vec2 PixelCoords;

uniform sampler2D edgesTexture;
uniform sampler2D areaTexture;
uniform sampler2D searchTexture;

uniform vec4 resolutionAndInverse;

const vec2 threshold = vec2(0.1);
const int maxSearchSteps = 16;
const int searchStepDiagonals = 8;
const int cornerRounding = 25;

#define saturate(a) clamp(a, 0.0, 1.0)
#define round(v) floor(v + 0.5)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0)
#define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0)

void SMAAMovc(bvec2 cond, inout vec2 variable, vec2 value) {
  if (cond.x) variable.x = value.x;
  if (cond.y) variable.y = value.y;
}

void SMAAMovc(bvec4 cond, inout vec4 variable, vec4 value) {
  SMAAMovc(cond.xy, variable.xy, value.xy);
  SMAAMovc(cond.zw, variable.zw, value.zw);
}

SMAACalculateDiagonalWeights(sampler2D edgesTex, sampler2D areaTex, vec2 texcoord, vec2 e, vec4 subsampleIndices);
float SMAASearchXLeft(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end);

void main() {
    vec4 subsampleIndices = vec4(0.0);
    vec4 weights = vec4(0.0);
    vec2 e = texture(edgesTexture, TexCoords).rg;

    if (e.g > 0.0) {
        weights.rg = SMAACalculateDiagonalWeights(edgesTexture, areaTexture, TexCoords, e, subsampleIndices);

        if (weights.r == -weights.g) {
            vec2 d;

            vec3 coords;
            coords.x = SMAASearchXLeft(edgesTexture, searchTexture, Offsets[0].xy, Offsets[2].x);
            coords.y = Offsets[1].y;
            d.x = coords.x;

            float e1 = texture(edgesTexture, coords.xy).r;

            coords.z = SMAASearchXRight(edgesTexture, searchTexture, Offsets[0].zw, Offsets[2].y);
            d.y = coords.z;

            d = abs(round(resolutionAndInverse.zz * d - PixelCoords.xx));

            vec2 sqrt_d = sqrt(d);
            float e2 = textureLodOffset(edgesTexture, coords.zy, 0, vec2(1, 0)).r;

            weights.rg = SMAAArea(areaTexture, sqrt_d, e1, e2, subsampleIndices.y);

            coords.y = TexCoords.y;
            SMAADetectHorizontalCornerPattern(edgesTexture, weights.rg, coords.xyzy, d);
        } else {
            e.r = 0.0;
        }
    }

    if (e.r > 0.0) {
        vec2 d;

        vec3 coords;
        coords.y = SMAASearchYUp(edgesTexture, searchTexture, Offsets[1].xy, Offsets[2].z);
        coords.x = Offsets[0].x;
        d.x = coords.y;

        float e1 = texture(edgesTexture, coords.xy).g;

        coords.z = SMAASearchYDown(edgesTexture, searchTexture, Offsets[1].zw, Offsets[2].w);
        d.y = coords.z;

        d = abs(round(resolutionAndInverse.ww * d - PixelCoords.yy));

        vec2 sqrt_d = sqrt(d);
        float e2 = textureLodOffset(edgesTexture, coords.xz, 0, vec2(0, 1)).g;

        weights.ba = SMAAArea(areaTexture, sqrt_d, e1, e2, subsampleIndices.x);

        coords.x = TexCoords.x;
        SMAADetectVerticalCornerPattern(edgesTexture, weights.ba, coords.xyxz, d);
    }

    FragColor = weights;
}

float SMAASearchLength(sampler2D searchTex, vec2 e, float offset) {
  // The texture is flipped vertically, with left and right cases taking half
  // of the space horizontally:
  vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0);
  vec2 bias = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0);

  // Scale and bias to access texel centers:
  scale += vec2(-1.0,  1.0);
  bias  += vec2( 0.5, -0.5);

  // Convert from pixel coordinates to texcoords:
  // (We use SMAA_SEARCHTEX_PACKED_SIZE because the texture is cropped)
  scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
  bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;

  // Lookup the search texture:
  vec2 coords = scale * e + bias;
  return texture(searchtex, coords).r;
}

float SMAASearchXLeft(sampler2D edgesTex, sampler2D searchTex, vec2 texcoord, float end) {
    vec2 e = vec2(0.0, 1.0);
    for (int i = 0; i < maxSearchSteps; i++) {
        if (!(texcoord.x > end && e.g > 0.8281 && e.r == 0.0)) break;
        e = texture2D(edgesTex, texcoord).rg; // LinearSampler
        
        texcoord = -vec2(2.0, 0.0) * resolutionAndInverse.xy + texcoord;
    }
    float offset = (-255.0 / 127.0) * SMAASearchLength(searchTex, e, 0.0) + 3.25;

    return resolutionAndInverse.x * offset + texcoord.x;
}

SMAACalculateDiagonalWeights(sampler2D edgesTex, sampler2D areaTex, vec2 texcoord, vec2 e, vec4 subsampleIndices) {
    vec2 weights = vec2(0.0);

    vec4 d;
    vec2 end;
    if (e.r > 0.0) {
      d.xz = SMAASearchDiag1(edgesTex, texcoord, vec2(-1.0,  1.0), end);
      d.x += float(end.y > 0.9);
    } else
        d.xz = vec2(0.0, 0.0);
    d.yw = SMAASearchDiag1(edgesTex, texcoord, vec2(1.0, -1.0), end);

    if (d.x + d.y > 2.0) {
    // Fetch the crossing edges:
    vec4 coords = mad(vec4(-d.x + 0.25, d.x, d.y, -d.y - 0.25), SMAA_RT_METRICS.xyxy, texcoord.xyxy);
    vec4 c;
    c.xy = textureLodOffset(edgesTex, coords.xy, 0, vec2(-1, 0)).rg;
    c.zw = textureLodOffset(edgesTex, coords.xy, 0, vec2(1, 0)).rg;
    c.yxwz = SMAADecodeDiagBilinearAccess(c.xyzw);

    // Merge crossing edges at each side into a single value:
    vec2 cc = vec2(2.0) * c.xz + c.yw;

    // Remove the crossing edge if we didn't found the end of the line:
    SMAAMovc(bvec2(step(0.9, d.zw)), cc, vec2(0.0));

    // Fetch the areas for this line:
    weights += SMAAAreaDiag(areaTex, d.xy, cc, subsampleIndices.z);
  }

  // Search for the line ends:
  d.xz = SMAASearchDiag2(edgesTex, texcoord, vec2(-1.0), end);
  if (textureLodOffset(edgesTex, texcoord, 0, vec2(1, 0)).r > 0.0) {
    d.yw = SMAASearchDiag2(edgesTex, texcoord, vec2(1.0), end);
    d.y += float(end.y > 0.9);
  } else {
    d.yw = vec2(0.0, 0.0);
  }

  if (d.x + d.y > 2.0) {
    // Fetch the crossing edges:
    vec4 coords = vec4(-d.x, -d.x, d.y, d.y) * resolutionAndInverse.xyxy + texcoord.xyxy;
    vec4 c;
    c.x = textureLodOffset(edgesTex, coords.xy, 0, vec2(-1, 0)).g;
    c.y = textureLodOffset(edgesTex, coords.xy, 0, vec2(0, -1)).r;
    c.zw = textureLodOffset(edgesTex, coords.zw, 0, vec2(1, 0)).gr;
    vec2 cc = vec2(2.0) * c.xz + c.yw;

    // Remove the crossing edge if we didn't found the end of the line:
    SMAAMovc(bvec2(step(0.9, d.zw)), cc, vec2(0.0, 0.0));

    // Fetch the areas for this line:
    weights += SMAAAreaDiag(areaTex, d.xy, cc, subsampleIndices.w).gr;
  }

  return weights;
}