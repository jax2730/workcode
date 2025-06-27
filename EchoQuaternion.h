//-----------------------------------------------------------------------------
// File:   EchoQuaternion.h
//
// Author: miao_yuzhuang
//
// Date:   2016-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoQuaternion_H__
#define __EchoQuaternion_H__

#include "EchoCommonPrerequisites.h"
#include "EchoMath.h"
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------


namespace Echo
{

	/** Implementation of a Quaternion, i.e. a rotation around an axis.
		For more information about Quaternions and the theory behind it, we recommend reading:
		http://www.ogre3d.org/tikiwiki/Quaternion+and+Rotation+Primer
		http://www.cprogramming.com/tutorial/3d/quaternions.html
		http://www.gamedev.net/page/resources/_/reference/programming/math-and-physics/
		quaternions/quaternion-powers-r1095
    */
    class _EchoCommonExport Quaternion
    {
		friend class Transform;
    public:
		/// Default constructor, initializes to identity rotation (aka 0°)
		inline Quaternion()
			: x(0), y(0), z(0), w(1)
		{
		}
	public:
		/// Construct from an explicit list of values
		inline Quaternion(
			float fX, float fY, float fZ, float fW)
			: x(fX), y(fY), z(fZ), w(fW)
		{
		}
	public:

        /// Construct a quaternion from a rotation matrix
        inline explicit  Quaternion(const Matrix3& rot)
        {
            this->FromRotationMatrix(rot);
        }
        /// Construct a quaternion from an angle/axis
        inline Quaternion(const Radian& rfAngle, const Vector3& rkAxis)
        {
            this->FromAngleAxis(rfAngle, rkAxis);
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline Quaternion(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis)
        {
            this->FromAxes(xaxis, yaxis, zaxis);
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline explicit Quaternion(const Vector3* akAxis)
        {
            this->FromAxes(akAxis);
        }


		/** Exchange the contents of this quaternion with another. 
		*/
		inline void swap(Quaternion& other)
		{
			std::swap(w, other.w);
			std::swap(x, other.x);
			std::swap(y, other.y);
			std::swap(z, other.z);
		}

		inline bool isEqual(const Quaternion& other, float tolerance = std::numeric_limits<float>::epsilon()) const
		{
			return (Math::FloatEqual(x, other.x,tolerance) && Math::FloatEqual(y, other.y,tolerance) && Math::FloatEqual(z, other.z,tolerance) && Math::FloatEqual(w, other.w,tolerance));
		}

		bool IsNan() const
		{
			return EchoIsNanOrInf(x) || EchoIsNanOrInf(y) || EchoIsNanOrInf(z) || EchoIsNanOrInf(w);
		}
	public:
		//Construct a quaternion from 4 manual w / x / y / z values
		inline Quaternion(float* valptr)
		{
			memcpy(&x, valptr, sizeof(float) * 4);
		}
		//Array accessor operator
		inline float operator [] (const size_t i) const
		{
			assert(i < 4);

			return *(&x + i);
		}

		//Array accessor operator
		inline float& operator [] (const size_t i)
		{
			assert(i < 4);

			return *(&x + i);
		}

		//Pointer accessor for direct copying
		inline float* ptr()
		{
			return &x;
		}

		//Pointer accessor for direct copying
		inline const float* ptr() const
		{
			return &x;
		}
	public:
		void FromAxesToAxes(const Vector3& from, const Vector3& to);

		void FromRotationMatrix (const Matrix3& kRot);
        void ToRotationMatrix (Matrix3& kRot) const;
		/** Setups the quaternion using the supplied vector, and "roll" around
			that vector by the specified radians.
		*/
        void FromAngleAxis (const Radian& rfAngle, const Vector3& rkAxis);
        void ToAngleAxis (Radian& rfAngle, Vector3& rkAxis) const;
        inline void ToAngleAxis (Degree& dAngle, Vector3& rkAxis) const {
            Radian rAngle;
            ToAngleAxis ( rAngle, rkAxis );
            dAngle = rAngle;
        }
		/** Constructs the quaternion using 3 axes, the axes are assumed to be orthonormal
			@see FromAxes
		*/
        void FromAxes (const Vector3* akAxis);
        void FromAxes (const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis);
		/** Gets the 3 orthonormal axes defining the quaternion. @see FromAxes */
        void ToAxes (Vector3* akAxis) const;
        void ToAxes (Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) const;

		//从欧拉角构造四元数
		void FromEuler(const Vector3& degree);

		//转换成欧拉角
		Vector3 ToEuler();
		//转换成欧拉角，范围0-360
		Vector3 ToEulerPositive();

		/** Returns the X orthonormal axis defining the quaternion. Same as doing
			xAxis = Vector3::UNIT_X * this. Also called the local X-axis
		*/
        Vector3 xAxis(void) const;

        /** Returns the Y orthonormal axis defining the quaternion. Same as doing
			yAxis = Vector3::UNIT_Y * this. Also called the local Y-axis
		*/
        Vector3 yAxis(void) const;

		/** Returns the Z orthonormal axis defining the quaternion. Same as doing
			zAxis = Vector3::UNIT_Z * this. Also called the local Z-axis
		*/
        Vector3 zAxis(void) const;

        inline Quaternion& operator= (const Quaternion& rkQ)
		{
			w = rkQ.w;
			x = rkQ.x;
			y = rkQ.y;
			z = rkQ.z;
			return *this;
		}
        Quaternion operator+ (const Quaternion& rkQ) const;
        Quaternion operator- (const Quaternion& rkQ) const;
        Quaternion operator* (const Quaternion& rkQ) const;
        Quaternion operator* (float fScalar) const;
        friend Quaternion operator* (float fScalar,
            const Quaternion& rkQ);
        Quaternion operator- () const;
        inline bool operator== (const Quaternion& rhs) const
		{
			return (rhs.x == x) && (rhs.y == y) &&
				(rhs.z == z) && (rhs.w == w);
		}
        inline bool operator!= (const Quaternion& rhs) const
		{
			return !operator==(rhs);
		}
        // functions of a quaternion
        /// Returns the dot product of the quaternion
        float Dot (const Quaternion& rkQ) const;
        /* Returns the normal length of this quaternion.
            @note This does <b>not</b> alter any values.
        */
        float Norm () const;
        /// Normalises this quaternion, and returns the previous length
        float normalise(void); 
        Quaternion Inverse () const;  // apply to non-zero quaternion
        Quaternion UnitInverse () const;  // apply to unit-length quaternion
        Quaternion Exp () const;
        Quaternion Log () const;

        /// Rotation of a vector by a quaternion
        Vector3 operator* (const Vector3& rkVector) const;
        DVector3 operator* (const DVector3& rkVector) const;

   		/** Calculate the local roll element of this quaternion.
		@param reprojectAxis By default the method returns the 'intuitive' result
			that is, if you projected the local Y of the quaternion onto the X and
			Y axes, the angle between them is returned. If set to false though, the
			result is the actual yaw that will be used to implement the quaternion,
			which is the shortest possible path to get to the same orientation and 
             may involve less axial rotation.  The co-domain of the returned value is 
             from -180 to 180 degrees.
		*/
		Radian getRoll(bool reprojectAxis = true) const;
   		/** Calculate the local pitch element of this quaternion
		@param reprojectAxis By default the method returns the 'intuitive' result
			that is, if you projected the local Z of the quaternion onto the X and
			Y axes, the angle between them is returned. If set to true though, the
			result is the actual yaw that will be used to implement the quaternion,
			which is the shortest possible path to get to the same orientation and 
            may involve less axial rotation.  The co-domain of the returned value is 
            from -180 to 180 degrees.
		*/
		Radian getPitch(bool reprojectAxis = true) const;
   		/** Calculate the local yaw element of this quaternion
		@param reprojectAxis By default the method returns the 'intuitive' result
			that is, if you projected the local Y of the quaternion onto the X and
			Z axes, the angle between them is returned. If set to true though, the
			result is the actual yaw that will be used to implement the quaternion,
			which is the shortest possible path to get to the same orientation and 
			may involve less axial rotation. The co-domain of the returned value is 
            from -180 to 180 degrees.
		*/
		Radian getYaw(bool reprojectAxis = true) const;		
		/// Equality with tolerance (tolerance is max angle difference)
		bool equals(const Quaternion& rhs, const Radian& tolerance) const;
		
	    /** Performs Spherical linear interpolation between two quaternions, and returns the result.
			Slerp ( 0.0f, A, B ) = A
			Slerp ( 1.0f, A, B ) = B
			@return Interpolated quaternion
			@remarks
			Slerp has the proprieties of performing the interpolation at constant
			velocity, and being torque-minimal (unless shortestPath=false).
			However, it's NOT commutative, which means
			Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A );
			therefore be careful if your code relies in the order of the operands.
			This is specially important in IK animation.
		*/
        static Quaternion Slerp (float fT, const Quaternion& rkP,
            const Quaternion& rkQ, bool shortestPath = false);

		static Quaternion ManualSlerp(float fT, const Quaternion& rkP,
			const Quaternion& rkQ, bool useLargeArc = false);
		/** @see Slerp. It adds extra "spins" (i.e. rotates several times) specified
			by parameter 'iExtraSpins' while interpolating before arriving to the
			final values
		*/
        static Quaternion SlerpExtraSpins (float fT,
            const Quaternion& rkP, const Quaternion& rkQ,
            int iExtraSpins);

        // setup for spherical quadratic interpolation
        static void Intermediate (const Quaternion& rkQ0,
            const Quaternion& rkQ1, const Quaternion& rkQ2,
            Quaternion& rka, Quaternion& rkB);

        // spherical quadratic interpolation
        static Quaternion Squad (float fT, const Quaternion& rkP,
            const Quaternion& rkA, const Quaternion& rkB,
            const Quaternion& rkQ, bool shortestPath = false);

        /** Performs Normalised linear interpolation between two quaternions, and returns the result.
			nlerp ( 0.0f, A, B ) = A
			nlerp ( 1.0f, A, B ) = B
			@remarks
			Nlerp is faster than Slerp.
			Nlerp has the proprieties of being commutative (@see Slerp;
			commutativity is desired in certain places, like IK animation), and
			being torque-minimal (unless shortestPath=false). However, it's performing
			the interpolation at non-constant velocity; sometimes this is desired,
			sometimes it is not. Having a non-constant velocity can produce a more
			natural rotation feeling without the need of tweaking the weights; however
			if your scene relies on the timing of the rotation or assumes it will point
			at a specific angle at a specific weight value, Slerp is a better choice.
		*/
        static Quaternion nlerp(float fT, const Quaternion& rkP, 
            const Quaternion& rkQ, bool shortestPath = false);

		static Quaternion nlerp_neon(float fT, const Quaternion& rkP,
			const Quaternion& rkQ, bool shortestPath = false);

        /// Cutoff for sine near zero
        static const float msEpsilon;

        // special values
        //static const Quaternion ZERO;
        static const Quaternion IDENTITY;

		float x, y, z, w;

		/// Check whether this quaternion contains valid values
		inline bool isNaN() const
		{
			return Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z) || Math::isNaN(w);
		}

        /** Function for writing to a stream. Outputs "Quaternion(w, x, y, z)" with w,x,y,z
            being the member values of the quaternion.
        */
		inline friend std::ostream& operator <<
			(std::ostream& o, const Quaternion& q)
		{
			o << "Quaternion(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")";
			return o;
		}

		void Conjugate()
		{
			x = -x;
			y = -y;
			z = -z;
		}

		void Reverse()
		{
			x = -x;
			y = -y;
			z = -z;
			w = -w;
		}

		//float Dot(const Quaternion &q) const
		//{
		//	return x * q.x + y * q.y + z * q.z + w * q.w;
		//}

		void Normalize()
		{
			float Len = Length();
			assert(Len >ECHO_EPSILON  && "Invalid orientation supplied as parameter");
			if (EchoAlmostZero(Len))
			{
				return;
			}
			float r = 1.0f / Len;
			x *= r;
			y *= r;
			z *= r;
			w *= r;
		}

		void Identity()
		{
			x = y = z = 0;
			w = 1.0f;
		}

		float LengthSq()
		{
			return x * x + y * y + z * z + w * w;
		}

		float Length()
		{
			return sqrtf(x * x + y * y + z * z + w * w);
		}

		bool Equals(const Quaternion& q, float radianTolerance = ECHO_ONE_RAD);

		//bool IsNan() const
		//{
		//	return EchoIsNanOrInf(x) || EchoIsNanOrInf(y) || EchoIsNanOrInf(z) || EchoIsNanOrInf(w);
		//}
		std::string ToString()
		{
			std::stringstream ss;
			ss << "Quat(" << x << ", " << y << ", " << z << ", " << w << ")";
			return ss.str();
		}

		inline void Multiply(const Quaternion& q)
		{
			float t_w = w * q.w - x * q.x - y * q.y - z * q.z;
			float t_x = w * q.x + x * q.w - y * q.z + z * q.y;
			float t_y = w * q.y + x * q.z + y * q.w - z * q.x;
			float t_z = w * q.z - x * q.y + y * q.x + z * q.w;
			x = t_x;
			y = t_y;
			z = t_z;
			w = t_w;
		}

		void Lerp(const Quaternion& q1, const Quaternion& q2, float t);
		static Quaternion QLerp(const Quaternion& q1, const Quaternion& q2, float t);

		void RotationAxisToAxis(const Vector3& _From, const Vector3& _To);
		void RotationX(float angle);
		void RotationY(float angle);
		void RotationZ(float angle);
		void RotationAxisAngle(const Vector3& _Axis, float _Angle);
		void ToAxisAngle(Vector3& axis, float& angle);
		void RotationMatrix(const Matrix3& m);
		void RotationMatrix(const Matrix4& m);
		void Slerp(const Quaternion& q1, const Quaternion& q2, float t);
		static Quaternion QSlerp(const Quaternion& q1, const Quaternion& q2, float t);
		//void Transform(const Matrix4& m);
		//void Inverse();
		Quaternion GetInverse() const;
		Quaternion GetNormalize()const;
		void ToEulerAnglesZXY(float& yaw, float& pitch, float& roll) const;
		void FromEulerAnglesZXY(float _Yaw, float _Pitch, float _Roll);
		float GetYawRadian() const;
		float GetPitchRadian() const;
		static float Dot(const Quaternion& a, const Quaternion& b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
		static float Angle(const Quaternion& a, const Quaternion& b);
		static Quaternion AngleAxis(float angle, const Vector3& axis);
		static Quaternion FromToRotation(const Vector3& from, const Vector3& to);
		static Quaternion LookRotation(const Vector3& forward, const Vector3& upwards);
		static Quaternion EulerToQuaternion(const Vector3& euler);
		static Quaternion RotateTowards(const Quaternion& from, const Quaternion& to, float maxDegreesDelta);
		static Quaternion QFromToRotation(const Quaternion& from,const Quaternion& to);
		static Quaternion ClampRotation(const Quaternion& rotation, float clampWeight, int clampSmoothing);
		static Quaternion QInverse(const Quaternion& rotation);
		static Quaternion convertWorldToLocalOrientation(const Quaternion& parentQuat, const Quaternion &childQuat);


		REGISTER_LUA_CLASS_KEY;

		private:
			bool MatrixToEuler_FromUnity(const Matrix3& matrix, Vector3& v);
			void QuaternionToMatrix_FromUnity(const Quaternion& q, Matrix3& m);
			inline void SanitizeEuler_FromUnity(Vector3& euler);
    };
	bool _EchoCommonExport LookRotationToQuaternion(const Vector3& viewVec, const Vector3& upVec, Quaternion& res);
	void _EchoCommonExport MatrixToQuaternion(const Matrix3& m, Quaternion& q);
	void _EchoCommonExport MatrixToQuaternion(const Matrix4& m, Quaternion& q);
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
