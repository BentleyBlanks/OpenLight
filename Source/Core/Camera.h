#pragma once
#include"Utility.h"
#include <DirectXMath.h>
using namespace DirectX;
class RoamCamera
{
	
public:
	struct Frustum
	{
		float NearZ;
		float FarZ;
		float Aspect;
		float FovY;
		float NearWindowHeight;
		float NearWindowWidth;
		float FarWindowHeight;
		float FarWindowWidth;
	};
	RoamCamera(const XMFLOAT3& look,
		const XMFLOAT3& up,
		const XMFLOAT3& pos, float nearZ, float farZ, int width, int height);
	~RoamCamera() {}




	Frustum		getFrustum() const { return mFrustum; }

	float			getNearZ() const { return mFrustum.NearZ; }
	float			getFarZ() const { return mFrustum.FarZ; }
	float			getFovY() const { return mFrustum.FovY; }
	float			getAspect() const { return mFrustum.Aspect; }

	float			getNearWindowHeight() const { return mFrustum.NearWindowHeight; }
	float			getNearWindowWidth()  const { return mFrustum.NearWindowWidth; }
	float			getFarWindowHeight() const { return mFrustum.FarWindowHeight; }
	float			getFarWindowWidth()	 const { return mFrustum.FarWindowWidth; }
	XMMATRIX	getView() const { return XMLoadFloat4x4(&mView); }
	XMMATRIX	getProj() const { return XMLoadFloat4x4(&mProj); }
	XMMATRIX	getViewProj() const { return getView()*getProj(); }

	void		setPos(float x, float y, float z) { mPos = XMFLOAT3(x, y, z); }
	void		setPos(const XMFLOAT3& pos) { mPos = pos; }

	void		setTarget(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	//666666666666666666666666666666666666666666666666666

	void		setLens(float fovY, float aspect, float nearZ, float farZ);


	void		updateMatrix();

	XMFLOAT3		getLook() { return mLook; }
	XMFLOAT3		getUp() { return mUp; }
	XMFLOAT3		getPos() { return mPos; }

	//look 方向平移
	void		walk(float dt);
	//right方向平移
	void		strafe(float dt);

	//绕right轴
	void		pitch(float angle);

	//绕up轴
	void		yaw(float angle);

	//绕y轴
	void		rotateY(float angle);

private:

	XMFLOAT3		mPos;
	XMFLOAT3		mLook;
	XMFLOAT3		mUp;
	XMFLOAT3		mRight;

	Frustum		mFrustum;

	XMFLOAT4X4	mView;
	XMFLOAT4X4	mProj;
};