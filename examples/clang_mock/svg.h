// clangd mock of the Scinc intrinsic "svg.h". Never read by Scinc: the
// interpreter handles this include itself. Signatures come from `Scinc -hdr`.
#ifndef SVG_H
#define SVG_H

int SVGLoad(const char*);
int SVGNewPose();
int SVGAddPoseTarget(int,const char*,const char*);
void SVGSetPoseTargetValue(int,int,float);
void SVGBindPose(int,int);
int SVGLoadAnimation(int,const char*);
int SVGLoad(const char*,const char*);
void SVGSetTransform(float,float,float,float,float,float);
void SVGDraw(int);
void SVGDrawAnimated(int,int,float);
void SVGDrawMultiAnimated(int,int,int*,float*,float*);
void SVGSetQuality(float);

#endif // SVG_H
