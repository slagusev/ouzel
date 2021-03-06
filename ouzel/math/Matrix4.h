// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#pragma once

#include "core/CompileConfig.h"
#if OUZEL_SUPPORTS_SSE
#include <xmmintrin.h>
#endif
#include "Vector3.h"
#include "Vector4.h"

namespace ouzel
{
    class Matrix4
    {
    public:
        static const Matrix4 IDENTITY;
        static const Matrix4 ZERO;

#if OUZEL_SUPPORTS_SSE
        union
        {
            __m128 col[4];
            float m[16];
        };
#else
        float m[16];
#endif

        Matrix4()
        {
            *this = Matrix4::ZERO;
        }

        Matrix4(float m11, float m12, float m13, float m14,
                float m21, float m22, float m23, float m24,
                float m31, float m32, float m33, float m34,
                float m41, float m42, float m43, float m44);

        Matrix4(const float* array);
        Matrix4(const Matrix4& copy);

        float& operator[](size_t index) { return m[index]; }
        float operator[](size_t index) const { return m[index]; }

        static void createLookAt(const Vector3& eyePosition, const Vector3& targetPosition, const Vector3& up, Matrix4& dst);
        static void createLookAt(float eyePositionX, float eyePositionY, float eyePositionZ,
                                 float targetCenterX, float targetCenterY, float targetCenterZ,
                                 float upX, float upY, float upZ, Matrix4& dst);
        static void createPerspective(float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane, Matrix4& dst);

        static void createOrthographicFromSize(float width, float height, float zNearPlane, float zFarPlane, Matrix4& dst);
        static void createOrthographicOffCenter(float left, float right, float bottom, float top,
                                                float zNearPlane, float zFarPlane, Matrix4& dst);
        static void createBillboard(const Vector3& objectPosition, const Vector3& cameraPosition,
                                    const Vector3& cameraUpVector, Matrix4& dst);
        static void createBillboard(const Vector3& objectPosition, const Vector3& cameraPosition,
                                    const Vector3& cameraUpVector, const Vector3& cameraForwardVector,
                                    Matrix4& dst);
        static void createScale(const Vector3& scale, Matrix4& dst);
        static void createScale(float xScale, float yScale, float zScale, Matrix4& dst);
        static void createRotation(const Vector3& axis, float angle, Matrix4& dst);
        static void createRotationX(float angle, Matrix4& dst);
        static void createRotationY(float angle, Matrix4& dst);
        static void createRotationZ(float angle, Matrix4& dst);
        static void createTranslation(const Vector3& translation, Matrix4& dst);
        static void createTranslation(float xTranslation, float yTranslation, float zTranslation, Matrix4& dst);

        void add(float scalar);
        void add(float scalar, Matrix4& dst);
        void add(const Matrix4& matrix);
        static void add(const Matrix4& m1, const Matrix4& m2, Matrix4& dst);

        float determinant() const;

        void getUpVector(Vector3& dst) const;
        void getDownVector(Vector3& dst) const;
        void getLeftVector(Vector3& dst) const;
        void getRightVector(Vector3& dst) const;
        void getForwardVector(Vector3& dst) const;
        void getBackVector(Vector3& dst) const;

        bool invert();
        bool invert(Matrix4& dst) const;

        bool isIdentity() const;

        void multiply(float scalar);
        void multiply(float scalar, Matrix4& dst) const;
        static void multiply(const Matrix4& m, float scalar, Matrix4& dst);
        void multiply(const Matrix4& matrix);
        static void multiply(const Matrix4& m1, const Matrix4& m2, Matrix4& dst);

        void negate();
        void negate(Matrix4& dst) const;

        void rotate(const Vector3& axis, float angle);
        void rotate(const Vector3& axis, float angle, Matrix4& dst) const;
        void rotateX(float angle);
        void rotateX(float angle, Matrix4& dst) const;
        void rotateY(float angle);
        void rotateY(float angle, Matrix4& dst) const;
        void rotateZ(float angle);
        void rotateZ(float angle, Matrix4& dst) const;

        void scale(float value);
        void scale(float value, Matrix4& dst) const;
        void scale(float xScale, float yScale, float zScale);
        void scale(float xScale, float yScale, float zScale, Matrix4& dst) const;
        void scale(const Vector3& s);
        void scale(const Vector3& s, Matrix4& dst) const;

        void set(float m11, float m12, float m13, float m14,
                 float m21, float m22, float m23, float m24,
                 float m31, float m32, float m33, float m34,
                 float m41, float m42, float m43, float m44);
        void set(const float* array);
        void setIdentity();
        void setZero();

        void subtract(const Matrix4& matrix);
        static void subtract(const Matrix4& m1, const Matrix4& m2, Matrix4& dst);

        void transformPoint(Vector3& point) const
        {
            transformVector(point.v[0], point.v[1], point.z(), 1.0f, point);
        }

        void transformPoint(const Vector3& point, Vector3& dst) const
        {
            transformVector(point.v[0], point.v[1], point.z(), 1.0f, dst);
        }

        void transformVector(Vector3& vector) const
        {
            Vector4 t;
            transformVector(Vector4(vector.v[0], vector.v[1], vector.z(), 0.0f), t);
            vector = Vector3(t.v[0], t.v[1], t.z());
        }

        void transformVector(const Vector3& vector, Vector3& dst) const
        {
            transformVector(vector.v[0], vector.v[1], vector.z(), 0.0f, dst);
        }

        void transformVector(float x, float y, float z, float w, Vector3& dst) const
        {
            Vector4 t;
            transformVector(Vector4(x, y, z, w), t);
            dst = Vector3(t.v[0], t.v[1], t.z());
        }

        void transformVector(Vector4& vector) const
        {
            transformVector(vector, vector);
        }

        void transformVector(const Vector4& vector, Vector4& dst) const;

        void translate(float x, float y, float z);
        void translate(float x, float y, float z, Matrix4& dst) const;
        void translate(const Vector3& t);
        void translate(const Vector3& t, Matrix4& dst) const;

        void transpose();
        void transpose(Matrix4& dst) const;

        inline Matrix4 operator+(const Matrix4& matrix) const
        {
            Matrix4 result(*this);
            result.add(matrix);
            return result;
        }

        inline Matrix4& operator+=(const Matrix4& matrix)
        {
            add(matrix);
            return *this;
        }

        inline Matrix4 operator-(const Matrix4& matrix) const
        {
            Matrix4 result(*this);
            result.subtract(matrix);
            return result;
        }

        inline Matrix4& operator-=(const Matrix4& matrix)
        {
            subtract(matrix);
            return *this;
        }

        inline Matrix4 operator-() const
        {
            Matrix4 result(*this);
            result.negate();
            return result;
        }

        inline Matrix4 operator*(const Matrix4& matrix) const
        {
            Matrix4 result(*this);
            result.multiply(matrix);
            return result;
        }

        inline Matrix4& operator*=(const Matrix4& matrix)
        {
            multiply(matrix);
            return *this;
        }

        inline bool operator==(const Matrix4& matrix)
        {
            return m[0] == matrix.m[0] &&
                   m[1] == matrix.m[1] &&
                   m[2] == matrix.m[2] &&
                   m[3] == matrix.m[3] &&
                   m[4] == matrix.m[4] &&
                   m[5] == matrix.m[5] &&
                   m[6] == matrix.m[6] &&
                   m[7] == matrix.m[7] &&
                   m[8] == matrix.m[8] &&
                   m[9] == matrix.m[9] &&
                   m[10] == matrix.m[10] &&
                   m[11] == matrix.m[11] &&
                   m[12] == matrix.m[12] &&
                   m[13] == matrix.m[13] &&
                   m[14] == matrix.m[14] &&
                   m[15] == matrix.m[15];
        }

        inline bool operator!=(const Matrix4& matrix)
        {
            return m[0] != matrix.m[0] ||
                   m[1] != matrix.m[1] ||
                   m[2] != matrix.m[2] ||
                   m[3] != matrix.m[3] ||
                   m[4] != matrix.m[4] ||
                   m[5] != matrix.m[5] ||
                   m[6] != matrix.m[6] ||
                   m[7] != matrix.m[7] ||
                   m[8] != matrix.m[8] ||
                   m[9] != matrix.m[9] ||
                   m[10] != matrix.m[10] ||
                   m[11] != matrix.m[11] ||
                   m[12] != matrix.m[12] ||
                   m[13] != matrix.m[13] ||
                   m[14] != matrix.m[14] ||
                   m[15] != matrix.m[15];
        }

    private:

        static void createBillboardHelper(const Vector3& objectPosition, const Vector3& cameraPosition,
                                          const Vector3& cameraUpVector, const Vector3& cameraForwardVector,
                                          Matrix4& dst);
    };

    inline Vector3& operator*=(Vector3& v, const Matrix4& m)
    {
        m.transformVector(v);
        return v;
    }

    inline Vector3 operator*(const Matrix4& m, const Vector3& v)
    {
        Vector3 x;
        m.transformVector(v, x);
        return x;
    }

    inline Vector4& operator*=(Vector4& v, const Matrix4& m)
    {
        m.transformVector(v);
        return v;
    }

    inline Vector4 operator*(const Matrix4& m, const Vector4& v)
    {
        Vector4 x;
        m.transformVector(v, x);
        return x;
    }
}
