#pragma once
#include <dinput.h>

class DebugCamera
{
public:
	struct Vector3
	{
		float x, y, z;
	};

	struct Matrix4x4
	{
		float m[4][4];
	};

	void Initialize();

	void Update(const BYTE* key);

	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

private:
	//Vector3 rotation_ = { 0, 0, 0 };
	Matrix4x4 matRot_;
	Vector3 translation_ = { 0, 0, -50 };

	Matrix4x4 viewMatrix_{};
	Matrix4x4 projectionMatrix_{};
};