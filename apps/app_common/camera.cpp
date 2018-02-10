#include "camera.h"
#include "core/misc.h"
#include "client/key_input.h"

Camera::Camera() {}

void Camera::Update(const Client::IInputProvider& input, f32 tick)
{
	if(input.WasMouseButtonPressed(0))
	{
		initialMousePos_ = input.GetMousePosition();
	}

	if(input.WasKeyReleased(Client::KeyCode::LEFT) || input.WasKeyReleased(Client::KeyCode::RIGHT))
		cameraRotationDelta_.y = 0.0f;

	if(input.WasKeyReleased(Client::KeyCode::UP) || input.WasKeyReleased(Client::KeyCode::DOWN))
		cameraRotationDelta_.x = 0.0f;

	if(input.WasKeyReleased('W') || input.WasKeyReleased('w') || input.WasKeyReleased('S') || input.WasKeyReleased('s'))
		cameraWalk_.z = 0.0f;

	if(input.WasKeyReleased('A') || input.WasKeyReleased('a') || input.WasKeyReleased('D') || input.WasKeyReleased('d'))
		cameraWalk_.x = 0.0f;

	if(input.WasKeyReleased(Client::KeyCode::LSHIFT))
		moveFast_ = false;
	if(input.WasKeyReleased(Client::KeyCode::LCTRL))
		moveSlow_ = false;


	if(input.WasKeyPressed(Client::KeyCode::LEFT))
		cameraRotationDelta_.y = 1.0f;
	if(input.WasKeyPressed(Client::KeyCode::RIGHT))
		cameraRotationDelta_.y = -1.0f;

	if(input.WasKeyPressed(Client::KeyCode::UP))
		cameraRotationDelta_.x = -1.0f;
	if(input.WasKeyPressed(Client::KeyCode::DOWN))
		cameraRotationDelta_.x = 1.0f;

	if(input.WasKeyPressed('W') || input.WasKeyPressed('w'))
		cameraWalk_.z = 1.0f;

	if(input.WasKeyPressed('S') || input.WasKeyPressed('s'))
		cameraWalk_.z = -1.0f;

	if(input.WasKeyPressed('A') || input.WasKeyPressed('a'))
		cameraWalk_.x = -1.0f;

	if(input.WasKeyPressed('D') || input.WasKeyPressed('d'))
		cameraWalk_.x = 1.0f;

	if(input.WasKeyPressed(Client::KeyCode::LSHIFT))
		moveFast_ = true;
	if(input.WasKeyPressed(Client::KeyCode::LCTRL))
		moveSlow_ = true;

	Math::Vec2 mousePos = input.GetMousePosition();
	Math::Vec2 mouseDelta = oldMousePos_ - mousePos;
	oldMousePos_ = mousePos;
	switch(cameraState_)
	{
	case CameraState::IDLE:
	{
	}
	break;

	case CameraState::ROTATE:
	{
		f32 rotateSpeed = 1.0f / 200.0f;
		Math::Vec3 cameraRotateAmount =
		    Math::Vec3(mousePos.y - initialMousePos_.y, -(mousePos.x - initialMousePos_.x), 0.0f) * rotateSpeed;
		cameraRotation_ = baseCameraRotation_ + cameraRotateAmount;
	}
	break;

	case CameraState::PAN:
	{
		f32 panSpeed = 4.0f;
		Math::Mat44 cameraRotationMatrix = GetCameraRotationMatrix();
		Math::Vec3 offsetVector = Math::Vec3(mouseDelta.x, mouseDelta.y, 0.0f) * cameraRotationMatrix;
		cameraTarget_ += offsetVector * tick * panSpeed;
	}
	break;
	}

	// Rotation.
	cameraRotation_ += cameraRotationDelta_ * tick * 4.0f;

	cameraDistance_ += cameraZoom_ * tick;
	cameraDistance_ = Core::Clamp(cameraDistance_, 1.0f, 4096.0f);
	cameraZoom_ = 0.0f;

	f32 walkSpeed = moveFast_ ? 128.0f : moveSlow_ ? 1.0f : 16.0f;
	Math::Mat44 cameraRotationMatrix = GetCameraRotationMatrix();
	Math::Vec3 offsetVector = -cameraWalk_ * cameraRotationMatrix;
	cameraTarget_ += offsetVector * tick * walkSpeed;


	Math::Vec3 viewDistance = Math::Vec3(0.0f, 0.0f, cameraDistance_);
	viewDistance = viewDistance * cameraRotationMatrix;
	Math::Vec3 viewFromPosition = cameraTarget_ + viewDistance;

	matrix_.Identity();
	matrix_.LookAt(viewFromPosition, cameraTarget_,
	    Math::Vec3(cameraRotationMatrix.Row1().x, cameraRotationMatrix.Row1().y, cameraRotationMatrix.Row1().z));
}

Math::Mat44 Camera::GetCameraRotationMatrix() const
{
	Math::Mat44 cameraPitchMatrix;
	Math::Mat44 cameraYawMatrix;
	Math::Mat44 cameraRollMatrix;
	cameraPitchMatrix.Rotation(Math::Vec3(cameraRotation_.x, 0.0f, 0.0f));
	cameraYawMatrix.Rotation(Math::Vec3(0.0f, cameraRotation_.y, 0.0f));
	cameraRollMatrix.Rotation(Math::Vec3(0.0f, 0.0f, cameraRotation_.z));
	return cameraRollMatrix * cameraPitchMatrix * cameraYawMatrix;
}
