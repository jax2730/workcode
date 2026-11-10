#pragma once

#include "EchoComponentPrerequisites.h"

#include "EchoName.h"
#include "PCG/Reflection/EchoPCGEnum.h"
#include "PCG/Reflection/EchoPCGProperty.h"
#include "PCG/Reflection/EchoPCGClass.h"
#include "PCG/Reflection/EchoPCGObjectV2.h"
#include "EchoSmartPtr.h"

namespace Echo
{
	class PCGComponent;
	using PCGComponentPtr = EchoWeakPtr<PCGComponent>;

	namespace PCGV2
	{
		typedef AxisAlignedBox FBox;
		typedef Vector3 FVector;

		class PCGSystem;
		class PCGSettings;
		class PCGElement;
		struct PCGContext;

		class PCGEdge;
		class PCGPin;
		class PCGNode;
		class PCGGraph;

		struct PCGPoint;
		class PCGPointData;

		struct PCGDataCollection;

		using PCGEdgePtr = std::shared_ptr<PCGEdge>;
		using UniquePCGPinPtr = std::unique_ptr<PCGPin>;
		using UniquePCGNodePtr = std::unique_ptr<PCGNode>;

		using PCGTaskId = uint64;
		static constexpr PCGTaskId c_InvalidTaskId = (uint64)-1;
		using PCGPinId = uint64;

		using PCGStackId = uint32;
		static constexpr PCGStackId c_InvalidStackId = (uint32)-1;

		// Data types supported by PCG nodes and pins
		enum class PCGDataType : uint32
		{
			None = 0,
			Point = 1 << 1,
			
			Spline = 1 << 2,
			LandscapeSpline = 1 << 3,
			PolyLine = Spline | LandscapeSpline,
			
			Landscape = 1 << 4,
			Texture = 1 << 5,
			RenderTarget = 1 << 6,
			BaseTexture = Texture | RenderTarget,
			Surface = Landscape | BaseTexture,
			
			Volume = 1 << 7,
			Primitive = 1 << 8,
			DynamicMesh = 1 << 10,
			
			/** Simple concrete data. */
			Concrete = Point | PolyLine | Surface | Volume | Primitive | DynamicMesh,
			
			/** Boolean operations like union, difference, intersection. */
			Composite = 1 << 9,
			
			/** Combinations of concrete data and/or boolean operations. */
			Spatial = Composite | Concrete,
			
			Param = 1 << 27,
			
			// Combination of Param and Point, necessary for named-based serialization of enums.
			PointOrParam = Point | Param,
			
			Settings = 1 << 28,
			Other = 1 << 29,
			Any = (1 << 30) - 1
		};
		ENUM_CLASS_FLAGS(PCGDataType)
		DECLARE_PCG_ENUM(PCGDataType)

		/**
		 * Combines two hash values to get a third.
		 * Note - this function is not commutative.
		 *
		 * This function cannot change for backward compatibility reasons.
		 * You may want to choose HashCombineFast for a better in-memory hash combining function.
		 */
		[[nodiscard]] inline uint32 HashCombine(uint32 A, uint32 C)
		{
			uint32 B = 0x9e3779b9;
			A += B;

			A -= B; A -= C; A ^= (C >> 13);
			B -= C; B -= A; B ^= (A << 8);
			C -= A; C -= B; C ^= (B >> 13);
			A -= B; A -= C; A ^= (C >> 12);
			B -= C; B -= A; B ^= (A << 16);
			C -= A; C -= B; C ^= (B >> 5);
			A -= B; A -= C; A ^= (C >> 3);
			B -= C; B -= A; B ^= (A << 10);
			C -= A; C -= B; C ^= (B >> 15);

			return C;
		}

		enum class PCGChangeType : uint32
		{
			None = 0,
			Cosmetic = 1 << 0,
			Settings = 1 << 1,
			Input = 1 << 2,
			Edge = 1 << 3,
			Node = 1 << 4
		};
		ENUM_CLASS_FLAGS(PCGChangeType)

		namespace PCGPinConstants
		{
			const Name DefaultInputLabel = Name("In");
			const Name DefaultOutputLabel = Name("Out");

			const Name DefaultInFilterLabel = Name("InsideFilter");
			const Name DefaultOutFilterLabel = Name("OutsideFilter");
		}

		void _EchoComponentExport PCG_LOG_BASE(const std::vector<const char*>& messages);
		void _EchoComponentExport PCG_LOG_BASE(const char* verbosity, PCGContext* context, const std::vector<const char*>& messages);

#define PCGE_LOG_C(Verbosity, LogMode, CustomContext, Message0, Message1) PCG_LOG_BASE(#Verbosity, CustomContext, {Message0, Message1})
#define PCGE_LOG(Verbosity, LogMode, Message0, Message1) PCGE_LOG_C(Verbosity, LogMode, Context, Message0, Message1)
	}

	namespace PCGV2 {

		class PCGBase;

		class _EchoComponentExport PCGBaseManager {
		public:
			void DestoryUnreferenced();
			void DestroyImpl(const PCGBase* ptr);
		public:
			std::list<const PCGBase*> gWaitDestroys;
			ECHO_AUTO_MUTEX
		};

		class _EchoComponentExport PCGBase
		{
		public:
			PCGBase() {}
			virtual ~PCGBase() {}

		public:
			inline int useCount() { return m_refrence_count.get(); }

			inline void addRef() { ++m_refrence_count; }

			inline void subRef() {
				if (--m_refrence_count == 0) {
					DestroyImpl();
				}
			}
		private:
			void DestroyImpl();
			AtomicScalar<unsigned int> m_refrence_count;
		};


		template <typename T>
		class TObjectPtr
		{
		public:
			using BASE = PCGBase;
		public:
			TObjectPtr() : mp_target(0) {}
			TObjectPtr(std::nullptr_t) : mp_target(0) {}
			~TObjectPtr()
			{
				setNull();
			}

			T* getPointer() const { return (T*)mp_target; }
			T* get() const { return (T*)mp_target; }  //for compability

			template <typename U>
			explicit TObjectPtr(U* ptr) : mp_target((BASE*)ptr) {
				if (mp_target) {
					mp_target->addRef();
				}
			}

			TObjectPtr(const TObjectPtr& rhs) : mp_target(0) {
				BASE* rhs_target = rhs.mp_target;
				if (rhs_target) {
					rhs_target->addRef();
					mp_target = rhs_target;
				}
			}

			template <typename U>
			TObjectPtr(const TObjectPtr<U>& rhs) : mp_target(0) {
				BASE* rhs_target = rhs.mp_target;
				if (rhs_target) {
					rhs_target->addRef();
					mp_target = rhs_target;
				}
			}

			TObjectPtr& operator = (const TObjectPtr& rhs) {
				setPointer(rhs.mp_target);
				return *this;
			}

			template <typename U>
			TObjectPtr& operator = (const TObjectPtr<U>& rhs) {
				setPointer(rhs.getPointer());
				return *this;
			}

			TObjectPtr& operator = (T* const new_target) {
				setPointer(new_target);
				return *this;
			}

			template <typename U>
			TObjectPtr& operator = (U* const new_target) {
				setPointer(new_target);
				return *this;
			}

			TObjectPtr& operator = (std::nullptr_t) {
				setNull();
				return *this;
			}

			inline bool operator == (std::nullptr_t) const {
				return isNull();
			}

			inline bool operator == (const TObjectPtr<T>& other) const {
				return getPointer() == other.getPointer();
			}

			template<typename U>
			inline bool operator == (const U* other) const {
				return getPointer() == other;
			}

			inline bool operator != (std::nullptr_t) const {
				return !isNull();
			}

			inline bool operator != (const TObjectPtr<T>& other) const {
				return getPointer() != other.getPointer();
			}

			template<typename U>
			inline bool operator != (const U* other) const {
				return getPointer() != other;
			}

			operator T* () const { return (T*)mp_target; }

			template<typename U>
			operator U* () const { return (U*)mp_target; }

			explicit operator bool() const { return !isNull(); }


			inline bool isNull() const {
				return mp_target == 0;
			}

			void swap(TObjectPtr<T>& rhs) {
				std::swap(mp_target, rhs.mp_target);
			}

			T& operator *() const { return *(T*)mp_target; }
			T* operator ->() const { return (T*)mp_target; }
		
			void setNull()
			{
				ECHO_LOCK_AUTO_MUTEX;
				if (mp_target)
				{
					mp_target->subRef();
					mp_target = 0;
				}
			}
		private:
			template<typename U>
			void setPointer(U* new_target) {
				BASE* new_pointer = (BASE*)new_target;
				if (new_pointer) {
					new_pointer->addRef();
				}
				setNull();
				mp_target = new_pointer;
			}
		private:
			BASE* mp_target;
			ECHO_AUTO_MUTEX;
		};


		template <typename T>
		inline void swap(TObjectPtr<T>& a, TObjectPtr<T>& b)
		{
			a.swap(b);
		}

		template <typename T, typename U>
		inline bool operator < (const TObjectPtr<T>& lhs, const TObjectPtr<U>& rhs) {
			return lhs.getPointer() < rhs.getPointer();
		}

		template <typename T, typename U>
		inline bool operator == (const TObjectPtr<T>& lhs, const TObjectPtr<U>& rhs) {
			return lhs.getPointer() == rhs.getPointer();
		}

		template <typename T, typename U>
		inline bool operator != (const TObjectPtr<T>& lhs, const TObjectPtr<U>& rhs) {
			return !(lhs == rhs);
		}

	}
}
