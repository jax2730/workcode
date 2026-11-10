#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"

namespace Echo
{
	class _EchoComponentExport EchoFlyingController : public Singleton<EchoFlyingController>
	{
	public:
		EchoFlyingController(Camera* camera);
		~EchoFlyingController();
		
		//just test for PadBo2
	public:
		void						updateFlyCCT(int type, float valx, float valy);
		void						createFlyCCT(bool enable);

	public:
		static bool					isCreated;
	private:
		Camera*						m_pCamera = nullptr;

	};
}