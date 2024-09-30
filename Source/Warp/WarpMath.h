#pragma once

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Matrix.h"
#include "Modules/ModuleManager.h"
#include "CoreTypes.h"
#include <algorithm>

using namespace std;

//Math function library for non-euqlidean transformations
//Quaternions implementation
//3D Mobius transform
//Used in hyperbolic and spherical geometry calculations

namespace WarpMath {

    inline FVector VectorUp() {
        return FVector(0, 0, 1);
    };

    inline FVector VectorLeft() {
        return FVector(0, -1, 0);
    };

    inline static void SetTileType(int n) {
        FWarpGameModule *m = &FModuleManager::GetModuleChecked<FWarpGameModule>("Warp");
        m->SetTileTypeW(n);
    };

    inline static float getK() { 
        FWarpGameModule *m = &FModuleManager::GetModuleChecked<FWarpGameModule>("Warp");
        return m->GetK();
    };

    inline static float getKV() {
        FWarpGameModule *m = &FModuleManager::GetModuleChecked<FWarpGameModule>("Warp");
        return m->GetKlein();
    };

    template<class T>
    constexpr const T& clamp(const T& lower, const T& upper, const T& n) {
        return lower >= n ? lower : n <= upper ? n : upper;
    }

    template<typename T> inline int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    inline static double sqrMagnitude(FVector a) {
        return a.X * a.X + a.Y * a.Y + a.Z * a.Z;
    }

    inline static FVector ClampMagnitude(FVector vector, float maxLength)
    {
        if ( sqrMagnitude(vector) > (double) maxLength * (double) maxLength) return vector.GetSafeNormal() * maxLength;
        return vector;
    }

    inline static FVector Project(FVector vector, FVector onNormal)
    {
        float sqrMag = FVector::DotProduct(onNormal, onNormal);
        if (sqrMag < DBL_EPSILON)
            return FVector(0, 0, 0);
        else
        {
            float dot = FVector::DotProduct(vector, onNormal);
            return FVector(onNormal.X * dot / sqrMag,
                onNormal.Y * dot / sqrMag,
                onNormal.Z * dot / sqrMag);
        }
    }

    inline FVector VLerp(FVector a, FVector b, double t) {
        return a * (1.0 - t) + b * t;
    }

    inline static FMatrix GetTranslationMatrix(FVector position)
    {
        return FMatrix(FPlane(1, 0, 0, 0),
            FPlane(0, 1, 0, 0),
            FPlane(0, 0, 1, 0),
            FPlane(position.X, position.Y, position.Z, 1)
        );
    }

    inline static FMatrix GetRotationMatrix(FVector anglesDeg)
    {
        anglesDeg = FVector(FMath::DegreesToRadians(anglesDeg[0]), FMath::DegreesToRadians(anglesDeg[1]), FMath::DegreesToRadians(anglesDeg[2]));

        FMatrix rotationX = FMatrix(FPlane(1, 0, 0, 0),
            FPlane(0, cos(anglesDeg[0]), sin(anglesDeg[0]), 0),
            FPlane(0, -sin(anglesDeg[0]), cos(anglesDeg[0]), 0),
            FPlane(0, 0, 0, 1));

        FMatrix rotationY = FMatrix(FPlane(cos(anglesDeg[1]), 0, -sin(anglesDeg[1]), 0),
            FPlane(0, 1, 0, 0),
            FPlane(sin(anglesDeg[1]), 0, cos(anglesDeg[1]), 0),
            FPlane(0, 0, 0, 1));

        FMatrix rotationZ = FMatrix(FPlane(cos(anglesDeg[2]), sin(anglesDeg[2]), 0, 0),
            FPlane(-sin(anglesDeg[2]), cos(anglesDeg[2]), 0, 0),
            FPlane(0, 0, 1, 0),
            FPlane(0, 0, 0, 1));

        return rotationX * rotationY * rotationZ;
    }

    inline static FMatrix GetScaleMatrix(FVector scale)
    {
        return FMatrix(FPlane(scale.X, 0, 0, 0),
            FPlane(0, scale.Y, 0, 0),
            FPlane(0, 0, scale.Z, 0),
            FPlane(0, 0, 0, 1));
    }

    inline static FMatrix TRS(FVector vec, FQuat gyr, FVector scale) {
        return GetTranslationMatrix(vec) * GetRotationMatrix(FVector(gyr.X, gyr.Y, gyr.Z)) * GetScaleMatrix(scale);
    }


    //Inverse hyperbolic trig functions
    inline double Acosh(double x) {
        return log(x + sqrt(x * x - 1));
    }

    inline double Atanh(double x) {
        return 0.5 * log((1.0 + x) / (1.0 - x));
    }

    //Curvature-dependent tangent
    inline float TanK(float x) {
        if (getK() > 0.0f) {
            return tan(x);
        }
        else if (getK() < 0.0f) {
            return (float)tanh(x);
        }
        else {
            return x;
        }
    }

    //Curvature-dependent inverse tangent
    inline float AtanK(float x) {
        if (getK() > 0.0f) {
            return atan(x);
        }
        else if (getK() < 0.0f) {
            return 0.5f * log((1.0f + x) / (1.0f - x));
        }
        else {
            return x;
        }
    }

    //3D Mobius add
    inline FVector MobiusAdd(FVector a, FVector b) {
        FVector c = getK() * FVector::CrossProduct(a, b);
        double d = 1.0 - getK() * FVector::DotProduct(a, b);
        FVector t = a + b;
        float cr = FVector::CrossProduct(c, t).X;
        return (t * d + FVector(cr, cr, cr)) / (d * d + sqrMagnitude(c));
    }

    //3D Mobius quat
    inline FQuat MobiusGyr(FVector a, FVector b) {
        FVector c = getK() * FVector::CrossProduct(a, b);
        float d = 1.0f - getK() * FVector::DotProduct(a, b);
        FQuat q = FQuat(c.X, c.Y, c.Z, d);
        q.Normalize();
        return q;
    }

    //3D Mobius sq dist
    inline double MobiusDistSq(FVector a, FVector b) {
        float a2 = sqrMagnitude(a);
        float b2 = sqrMagnitude(b);
        double ab = 2.0 * FVector::DotProduct(a, b);
        return (a2 - ab + b2) / (1.0 + getK() * (ab + getK() * a2 * b2));
    }

    //Transform Klein to Poincare
    inline FVector KleinToPoincare(FVector p) {
        if (getK() == 0.0f) { return p; }
        return p / (sqrt(std::max(0.0, 1.0 + getK() * sqrMagnitude(p))) + 1.0);
    }

    inline FVector PoincareToKlein(FVector p) {
        if (getK() == 0.0f) { return p; }
        return p * 2.0f / (1.0f - getK() * sqrMagnitude(p));
    }

    inline FVector KleinToPoincare(FVector p, FVector n) {
        if (getK() == 0.0f) { return n.GetUnsafeNormal(); }
        return ((1.0f + sqrt(1.0f + getK() * sqrMagnitude(p))) * n + (getK() * FVector::DotProduct(n, p)) * p).GetUnsafeNormal();
    }
	
    inline FVector PoincareToKlein(FVector p, FVector n) {
        if (getK() == 0.0f) { return n.GetUnsafeNormal(); }
        return ((1.0f + getK() * sqrMagnitude(p)) * n - (2.0f * getK() * FVector::DotProduct(n, p)) * p).GetUnsafeNormal();
    }

    inline FVector UnitToKlein(FVector p, bool useTanKHeight) {
        p *= getKV();
        if (useTanKHeight) {
            p.Y = TanK(p.Y) * sqrt(1.0f + getK() * (p.X * p.X + p.Z * p.Z));
        }
        return p;
    }
    inline FVector KleinToUnit(FVector p, bool useTanKHeight) {
        if (useTanKHeight) {
            p.Y = AtanK(p.Y / sqrt(1.0f + getK() * (p.X * p.X + p.Z * p.Z)));
        }
        return p / getKV();
    }

    inline FVector UnitToPoincare(FVector u, bool useTanKHeight) {
        return KleinToPoincare(UnitToKlein(u, useTanKHeight));
    }
    inline FVector PoincareToUnit(FVector u, bool useTanKHeight) {
        return KleinToUnit(PoincareToKlein(u), useTanKHeight);
    }

    inline float UnitToPoincareScale(FVector u, float r, bool useTanKHeight) {
        if (getK() == 0.0f) { return r; }
        u = UnitToKlein(u, useTanKHeight);
        float p = sqrt(1.0f + getK() * sqrMagnitude(u));
        return r * getKV() / (p * (p + 1));
    }
	
    inline float PoincareScaleFactor(FVector p) {
        return 1.0f + getK() * sqrMagnitude(p);
    }

    inline FVector HyperTranslate(FVector d) {
        float mag = d.Size();
        if (mag < 1e-5f) {
            return FVector(0, 0, 0);
        }
        return d * (TanK(mag) / mag);
    }
	
    inline FVector HyperTranslate(float dx, float dz) {
        return HyperTranslate(FVector(dx, 0.0f, dz));
    }
	
    inline FVector HyperTranslate(float dx, float dy, float dz) {
        return HyperTranslate(FVector(dx, dy, dz));
    }

    inline float PoincareDist(FVector a, FVector b) {
        return sqrMagnitude(a - b) / ((getK() + sqrMagnitude(a)) * (getK() + sqrMagnitude(b)));
    }

    inline static FVector UpVector(FVector p) {
        float u = 1.0f + getK() * sqrMagnitude(p);
        float v = -2.0f * getK() * p.Y;
        return (u * VectorUp() + v * p).GetUnsafeNormal();
    }

    inline static FQuat SwingTwist(FQuat q, FVector d) {
        FVector ra = FVector(q.X, q.Y, q.Z);
        FVector p = Project(ra, d);
        return FQuat(p.X, p.Y, p.Z, q.W).GetNormalized();
    }
	
    inline static FVector ProjectToPlaneV(FVector p) {
        double m = getK() * sqrMagnitude(p);
        double d = 1.0 + m;
        double s = 2.0 / (1.0 - m + sqrt(d * d - 4.0 * getK() * p.Y * p.Y));
        return FVector(p.X * s, 0.0, p.Z * s);
    }

    inline void MobiusAddGyrUnnorm(FVector a, FVector b, FVector* sum, FQuat* gyr) {
        FVector c = getK() * FVector::CrossProduct(a, b);
        float d = 1.0f - getK() * FVector::DotProduct(a, b);
        FVector t = a + b;
        float cr = FVector::CrossProduct(c, t).X;
        *sum = (t * d + FVector(cr, cr, cr)) / (d * d + sqrMagnitude(c));
        *gyr = FQuat(-c.X, -c.Y, -c.Z, d);
    }

    inline void MobiusAddGyr(FVector a, FVector b, FVector* sum, FQuat* gyr) {
        MobiusAddGyrUnnorm(a, b, sum, gyr);
        gyr->Normalize();
    }

    //Gyrovector structure stores Mobius transform
    struct GyroVectorD {

        //Members
        FVector vec;     //This is the hyperbolic offset vector or position
        FQuat gyr;  //This is the post-rotation as a result of holonomy

        //Constructors
        GyroVectorD() { vec = FVector(0, 0, 0); gyr = FQuat(0, 0, 0, 0); }
        GyroVectorD(double x, double y, double z) { vec = FVector(x, y, z); gyr = FQuat(0, 0, 0, 0); }
        GyroVectorD(FVector _vec) { vec = _vec; gyr = FQuat(0, 0, 0, 0); }
        GyroVectorD(FQuat _gyr) { vec = FVector(0, 0, 0); gyr = _gyr.GetNormalized(); }
        GyroVectorD(FVector _vec, FQuat _gyr) { vec = _vec; gyr = _gyr.GetNormalized(); }


        FVector Point() {
            return gyr * vec;
        }

        //Aligns the rotation of the gyrovector so that up is up
        void AlignUpVector() {
            FVector newAxis = UpVector(vec);

            FVector c = newAxis ^ VectorUp();
            double _w = sqrt(sqrMagnitude(newAxis) * sqrMagnitude(VectorUp())) + FVector::DotProduct(newAxis, VectorUp());
            FQuat newBasis = FQuat(c.X, c.Y, c.Z, _w).GetNormalized();

            FQuat twist = SwingTwist(gyr, newAxis);
            gyr = newBasis * twist;
        }

        //Projects the Gyrovector to the ground plane
        GyroVectorD ProjectToPlane() {
            //Remove the y-component from the Klein projection and any out-of-plane rotation
            return GyroVectorD(ProjectToPlaneV(vec), FQuat(0.0f, gyr.Y, 0.0f, gyr.W));
        }

        //Convert to a matrix so the shader can read it
        FMatrix ToMatrix() {
            return TRS(vec, gyr, FVector(1, 1, 1));
        }

    };

    inline void TransformNormal(FQuat gyr, FVector vec, FVector pt, FVector n, FVector* newPt, FVector* newN) {
        FVector v;
        FQuat q;
        MobiusAddGyr(vec, pt, &v, &q);
        *newPt = gyr * v;
        FVector fv = q.Inverse() * n;
        *newN = gyr * fv;
    }

    inline GyroVectorD add(GyroVectorD gv, FVector delta) {
        FVector newVec;
        FQuat newGyr;
        MobiusAddGyr(gv.vec, gv.gyr.Inverse()  * delta, &newVec, &newGyr);
        return GyroVectorD(newVec, gv.gyr * newGyr);
    }
	
    inline GyroVectorD add(FVector delta, GyroVectorD gv) {
        FVector newVec;
        FQuat newGyr;
        MobiusAddGyr(delta, gv.vec, &newVec, &newGyr);
        return GyroVectorD(newVec, gv.gyr * newGyr);
    }
	
    inline GyroVectorD add(GyroVectorD gv, FQuat rot) {
        return GyroVectorD(gv.vec, rot * gv.gyr);
    }
	
    inline GyroVectorD add(FQuat rot, GyroVectorD gv2) {
        return GyroVectorD(rot.Inverse() * gv2.vec, gv2.gyr * rot);
    }
	
    inline GyroVectorD add(GyroVectorD gv1, GyroVectorD gv2) {
        FVector newVec;
        FQuat newGyr;
        MobiusAddGyr(gv1.vec, gv1.gyr.Inverse() * gv2.vec, &newVec, &newGyr);

        FQuat q = gv2.gyr * gv1.gyr;
        return GyroVectorD(newVec, q * newGyr);
    }

    //Inverse GyroVectorD
    inline GyroVectorD InverseG(GyroVectorD gv) {
        return GyroVectorD(-(gv.gyr * gv.vec), gv.gyr.Inverse());
    }

    //Inverse composition
    inline GyroVectorD sub(GyroVectorD gv, FVector delta) {
        return add(gv, (-delta));
    }
    inline GyroVectorD sub(FVector delta, GyroVectorD gv) {
        return add(delta, InverseG(gv));
    }
    inline GyroVectorD sub(GyroVectorD gv, FQuat rot) {
        return add(gv, rot.Inverse());
    }
    inline GyroVectorD sub(FQuat rot, GyroVectorD gv) {
        return add(rot, InverseG(gv));
    }
    inline GyroVectorD sub(GyroVectorD gv1, GyroVectorD gv2) {
        return add(gv1, InverseG(gv2));
    }

    //Apply the full GyroVectorD to a point
    inline FVector apply(GyroVectorD gv, FVector pt) {
        return gv.gyr * MobiusAdd(gv.vec, pt);
    }

    //Spherical linear interpolation
    inline GyroVectorD Slerp(GyroVectorD a, GyroVectorD b, float t) {
        return GyroVectorD(VLerp(a.vec, b.vec, t), FQuat::FastLerp(a.gyr, b.gyr, t));
    }
    inline GyroVectorD SlerpReverse(GyroVectorD a, GyroVectorD b, float t) {
        return add(Slerp(FQuat(0, 0, 0, 0), sub(b, a), t), a);
    }

}