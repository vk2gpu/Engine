#include "common.h"
#include "client/input_provider.h"

class APP_COMMON_DLL Camera
{
public:
	enum class CameraState
	{
		IDLE = 0,
		ROTATE,
		PAN
	};

	Camera();
	void Update(const Client::IInputProvider& input, f32 tick);
	Math::Mat44 GetCameraRotationMatrix() const;

	CameraState cameraState_ = CameraState::IDLE;
	Math::Vec3 baseCameraRotation_ = Math::Vec3(0.0f, 0.0f, 0.0f);
	Math::Vec3 cameraTarget_ = Math::Vec3(0.0f, 5.0f, 5.0f);
	Math::Vec3 cameraRotation_ = Math::Vec3(0.0f, 0.0f, 0.0f);
	Math::Vec3 cameraWalk_ = Math::Vec3(0.0f, 0.0f, 0.0f);
	Math::Vec3 cameraRotationDelta_ = Math::Vec3(0.0f, 0.0f, 0.0f);
	f32 cameraDistance_ = 1.0f;
	f32 cameraZoom_ = 0.0f;
	bool moveFast_ = false;
	bool moveSlow_ = false;

	Math::Vec2 initialMousePos_;
	Math::Vec2 oldMousePos_;

	Math::Mat44 matrix_;
};
