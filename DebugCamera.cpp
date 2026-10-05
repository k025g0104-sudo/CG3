#include "DebugCamera.h"
#include <cmath>
#include <algorithm>

namespace
{
	using Vector3 = DebugCamera::Vector3;
	using Matrix4x4 = DebugCamera::Matrix4x4;

	Matrix4x4 MakeIdentityMatrix4x4()
	{
		Matrix4x4 result =
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
		return result;
	}

	Matrix4x4 MakeScaleMatrix(const Vector3& scale)
	{
		return
		{
			scale.x, 0.0f, 0.0f, 0.0f,
			0.0f, scale.y, 0.0f, 0.0f,
			0.0f, 0.0f, scale.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	Matrix4x4 MakeTranslationMatrix(const Vector3& translate)
	{
		return
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			translate.x, translate.y, translate.z, 1.0f
		};
	}

	Matrix4x4 RotateX(float angle)
	{
		Matrix4x4 result = MakeIdentityMatrix4x4();
		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		result.m[1][1] = cosA;
		result.m[1][2] = sinA;
		result.m[2][1] = -sinA;
		result.m[2][2] = cosA;
		return result;
	}

	Matrix4x4 RotateY(float angle)
	{
		Matrix4x4 result = MakeIdentityMatrix4x4();
		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		result.m[0][0] = cosA;
		result.m[0][2] = -sinA;
		result.m[2][0] = sinA;
		result.m[2][2] = cosA;
		return result;
	}

	Matrix4x4 RotateZ(float angle)
	{
		Matrix4x4 result = MakeIdentityMatrix4x4();
		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		result.m[0][0] = cosA;
		result.m[0][1] = sinA;
		result.m[1][0] = -sinA;
		result.m[1][1] = cosA;
		return result;
	}

	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
	{
		Matrix4x4 result = {};
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = 0.0f;
				for (int k = 0; k < 4; k++)
				{
					result.m[i][j] += m1.m[i][k] * m2.m[k][j];
				}
			}
		}
		return result;
	}

	Matrix4x4 Inverse(const Matrix4x4& m)
	{
		float mat[4][8];
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				mat[i][j] = m.m[i][j];
				mat[i][j + 4] = (i == j) ? 1.0f : 0.0f;
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			int pivot = i;
			for (int j = i + 1; j < 4; ++j)
			{
				if (std::abs(mat[j][i]) > std::abs(mat[pivot][i])) {
					pivot = j;
				}
			}

			for (int j = 0; j < 8; ++j)
			{
				std::swap(mat[i][j], mat[pivot][j]);
			}

			if (std::abs(mat[i][i]) < 1e-6f)
			{
				return MakeIdentityMatrix4x4();
			}

			float factor = mat[i][i];
			for (int j = 0; j < 8; ++j)
			{
				mat[i][j] /= factor;
			}

			for (int k = 0; k < 4; ++k)
			{
				if (k != i)
				{
					float subFactor = mat[k][i];
					for (int j = 0; j < 8; ++j)
					{
						mat[k][j] -= subFactor * mat[i][j];
					}
				}
			}
		}

		Matrix4x4 result;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result.m[i][j] = mat[i][j + 4];
			}
		}
		return result;
	}

	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
	{
		Matrix4x4 result = { 0 };
		float tanHalfFovY = std::tan(fovY / 2.0f);

		result.m[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
		result.m[1][1] = 1.0f / tanHalfFovY;
		result.m[2][2] = farClip / (farClip - nearClip);
		result.m[2][3] = 1.0f;
		result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

		return result;
	}

	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m)
	{
		Vector3 result;
		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
		result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
		return result;
	}

	constexpr int32_t kClientWidth = 1280;
	constexpr int32_t kClientHeight = 720;
}

void DebugCamera::Initialize()
{
	translation_ = { 0.0f, 0.0f, -50.0f };
	matRot_ = MakeIdentityMatrix4x4();  // 累積回転行列は単位行列で初期化

	viewMatrix_ = MakeIdentityMatrix4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(
		0.45f,
		static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight),
		0.1f, 100.0f);
}

void DebugCamera::Update(const BYTE* key, const DIMOUSESTATE2& mouse)
{
	const float moveSpeed = 0.5f;
	const float rotSpeed = 0.02f;
	const float mouseRotSpeed = 0.0005f; 
	const float wheelZoomSpeed = 0.05f; 

	if (key[DIK_Q])
	{
		Vector3 move = TransformNormal({ 0, 0, moveSpeed }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}
	if (key[DIK_E])
	{
		Vector3 move = TransformNormal({ 0, 0, -moveSpeed }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}

	if (key[DIK_D])
	{
		Vector3 move = TransformNormal({ -moveSpeed, 0, 0 }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}
	if (key[DIK_A])
	{
		Vector3 move = TransformNormal({ moveSpeed, 0, 0 }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}


	if (key[DIK_W])
	{
		Vector3 move = TransformNormal({ 0, -moveSpeed, 0 }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}
	if (key[DIK_S])
	{
		Vector3 move = TransformNormal({ 0, moveSpeed, 0 }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}

	if (mouse.lZ != 0)
	{
		Vector3 move = TransformNormal({ 0, 0, mouse.lZ * wheelZoomSpeed }, matRot_);
		translation_.x += move.x; translation_.y += move.y; translation_.z += move.z;
	}

	Matrix4x4 translationMatrix = MakeTranslationMatrix(translation_);
	Matrix4x4 worldMatrix = Multiply(matRot_, translationMatrix);

	viewMatrix_ = Inverse(worldMatrix);

	projectionMatrix_ = MakePerspectiveFovMatrix(
		0.45f,
		static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight),
		0.1f, 100.0f);
}