#include<Camera.h>

RoamCamera::RoamCamera(const XMFLOAT3 & look,
	const XMFLOAT3 & up,
	const XMFLOAT3 & pos, float nearZ, float farZ, int width, int height) :
	mLook(look), mUp(up), mPos(pos)
{
	mRight = normalize(cross(up, look));
	mUp = normalize(cross(mLook, mRight));

	setLens(2.f * XM_PI / 9.f, (float)width / (float)height, nearZ, farZ);
}


void RoamCamera::setLens(float fovY, float aspect, float nearZ, float farZ)
{
	mFrustum.FovY = fovY;
	mFrustum.Aspect = aspect;
	mFrustum.NearZ = nearZ;
	mFrustum.FarZ = farZ;

	mFrustum.NearWindowHeight = 2 * nearZ*tanf(fovY*0.5);
	mFrustum.FarWindowHeight = 2 * farZ*tanf(fovY*0.5);

	mFrustum.NearWindowWidth = aspect * mFrustum.NearWindowHeight;
	mFrustum.FarWindowWidth = aspect * mFrustum.FarWindowHeight;

	XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
	DirectX::XMStoreFloat4x4(&mProj, P);
}

void RoamCamera::updateMatrix()
{
	XMVECTOR	pos = XMLoadFloat3((XMFLOAT3*)&mPos);
	XMVECTOR	look = XMLoadFloat3((XMFLOAT3*)&mLook);
	XMVECTOR	up = XMLoadFloat3((XMFLOAT3*)&mUp);
	XMVECTOR	right = XMLoadFloat3((XMFLOAT3*)&mRight);

	look = XMVector3Normalize(look);

	up = XMVector3Normalize(XMVector3Cross(look, right));

	right = XMVector3Normalize(XMVector3Cross(up, look));

	float x = -XMVectorGetX(XMVector3Dot(pos, right));
	float y = -XMVectorGetX(XMVector3Dot(pos, up));
	float z = -XMVectorGetX(XMVector3Dot(pos, look));

	XMStoreFloat3((XMFLOAT3*)&mPos, pos);
	XMStoreFloat3((XMFLOAT3*)&mRight, right);
	XMStoreFloat3((XMFLOAT3*)&mUp, up);
	XMStoreFloat3((XMFLOAT3*)&mLook, look);

	mView(0, 0) = mRight.x;
	mView(1, 0) = mRight.y;
	mView(2, 0) = mRight.z;
	mView(3, 0) = x;

	mView(0, 1) = mUp.x;
	mView(1, 1) = mUp.y;
	mView(2, 1) = mUp.z;
	mView(3, 1) = y;

	mView(0, 2) = mLook.x;
	mView(1, 2) = mLook.y;
	mView(2, 2) = mLook.z;
	mView(3, 2) = z;

	mView(0, 3) = 0;
	mView(1, 3) = 0;
	mView(2, 3) = 0;
	mView(3, 3) = 1.0f;


}

void RoamCamera::walk(float dt)
{

	XMVECTOR dis = XMVectorReplicate(dt);
	XMVECTOR pos = XMLoadFloat3((XMFLOAT3*)&mPos);

	XMVECTOR look = XMLoadFloat3((XMFLOAT3*)&mLook);

	XMStoreFloat3((XMFLOAT3*)&mPos, XMVectorMultiplyAdd(dis, look, pos));
}

void RoamCamera::strafe(float dt)
{
	XMVECTOR dis = XMVectorReplicate(dt);
	XMVECTOR pos = XMLoadFloat3((XMFLOAT3*)&mPos);

	XMVECTOR right = XMLoadFloat3((XMFLOAT3*)&mRight);

	XMStoreFloat3((XMFLOAT3*)&mPos, XMVectorMultiplyAdd(dis, right, pos));
}

void RoamCamera::pitch(float angle)
{
	XMMATRIX r = DirectX::XMMatrixRotationAxis(XMLoadFloat3((XMFLOAT3*)&mRight), angle);

	XMStoreFloat3((XMFLOAT3*)&mUp, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mUp), r));
	XMStoreFloat3((XMFLOAT3*)&mLook, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mLook), r));

}

void RoamCamera::yaw(float angle)
{
	XMMATRIX r = DirectX::XMMatrixRotationAxis(XMLoadFloat3((XMFLOAT3*)&mUp), angle);

	XMStoreFloat3((XMFLOAT3*)&mRight, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mRight), r));
	XMStoreFloat3((XMFLOAT3*)&mLook, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mLook), r));
}

void RoamCamera::rotateY(float angle)
{
	XMMATRIX r = DirectX::XMMatrixRotationY(angle);

	XMStoreFloat3((XMFLOAT3*)&mRight, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mRight), r));
	XMStoreFloat3((XMFLOAT3*)&mLook, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mLook), r));
	XMStoreFloat3((XMFLOAT3*)&mUp, XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&mUp), r));
}

void RoamCamera::setTarget(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	mPos = pos;
	XMVECTOR p = XMLoadFloat3((XMFLOAT3*)&pos);
	XMVECTOR u = XMLoadFloat3((XMFLOAT3*)&up);
	XMVECTOR t = XMLoadFloat3((XMFLOAT3*)&target);
	XMVECTOR l = XMVector3Normalize(XMLoadFloat3((XMFLOAT3*)&(target - pos)));

	XMVECTOR r = XMVector3Cross(u, l);
	u = XMVector3Normalize(XMVector3Cross(l, r));

	XMStoreFloat3((XMFLOAT3*)&mLook, XMVector3Normalize(l));
	XMStoreFloat3((XMFLOAT3*)&mUp, XMVector3Normalize(u));
	XMStoreFloat3((XMFLOAT3*)&mRight, XMVector3Normalize(r));



}