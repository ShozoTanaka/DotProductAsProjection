#include "pch.h"
#include <memory>
#include <utility>
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include "GraphScene.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Model.h"
#include "Game.h"
#include "SpriteString2D.h"

// コンストラクタ
GraphScene::GraphScene(Game* game)
	:
	m_game(game),									// Gameクラス
	m_graphics(nullptr),							// DirectXGraphicsクラス
	m_device(nullptr),								// Deviceクラス
	m_context(nullptr),								// DeviceContextクラス
	m_keyboardState{},								// キーボードステート
	m_mouseCursorPosition{},					// マウスカーソル位置
	m_mouseState{},									// マウスステート
	m_mouseStateTracker{},						// マウスステートトラッカー
	m_worldMatrix{},									// ワールド
	m_viewMatrix{},									// ビュー
	m_projectionMatrix{},							// プロジェクション
	m_cameraRotation{},							// カメラ回転
	m_cameraPosition{},							// カメラ位置
	m_cameraFocus{},								// カメラフォーカス
	m_rotaionAngle(0.0f),							// 角度
	m_distance(10.0f),								// 注視点から視点までの距離
	m_zoom(1.0f),										// ズーム
	m_fov(DirectX::XM_PI / 4.0f),				// フィールドオブビュー
	m_nearPlane(0.1f),								// ニアクリップ
	m_farPlane(0.0f),								// ファークリップ
	m_scale(1.0f),										// スケール
	m_angleV1(0.0f),
	m_angleV2(0.0f),
	m_vectorV1(DirectX::SimpleMath::Vector2::Zero),
	m_vectorV2(DirectX::SimpleMath::Vector2::Zero),
	m_projection(0.0f)
{
	// DirectX Graphicsクラスのインスタンスを取得する
	m_graphics = Graphics::GetInstance();
	// デバイスを取得する
	m_device = Graphics::GetInstance()->GetDeviceResources()->GetD3DDevice();
	// デバイスコンテキストを取得する
	m_context = Graphics::GetInstance()->GetDeviceResources()->GetD3DDeviceContext();
}

// デストラクタ
GraphScene::~GraphScene()
{
}

// 初期化する
void GraphScene::Initialize()
{
}

// 更新する
void GraphScene::Update(const DX::StepTimer& timer)
{
	// キーボードの状態を取得する
	m_keyboardState = m_game->GetKeyboard()->GetState();
	// キーボードの状態を更新する
	m_game->GetKeyboardTracker().Update(m_keyboardState);

	// マウスの状態を取得する
	m_mouseState = m_game->GetMouse()->GetState();
	// マウストラッカーを更新する
	m_mouseStateTracker.Update(m_mouseState);

	// 左右キーでベクトルv1の方向を変える
	if (m_keyboardState.Left)
		m_angleV1 -= 1.0f;
	if (m_keyboardState.Right)
		m_angleV1 += 1.0f;
	// 上下キーでベクトルv2の方向を変える
	if (m_keyboardState.Up)
		m_angleV2 -= 1.0f;
	if (m_keyboardState.Down)
		m_angleV2 += 1.0f;

	// 向きを設定する
	DirectX::SimpleMath::Vector2 directionV1(1.414f, 0.0f);
	// 回転行列を生成する
	DirectX::SimpleMath::Matrix rotationV1 = DirectX::SimpleMath::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(m_angleV1));
	// ベクトルv1に方向と大きさ(50)を設定する
	m_vectorV1 = DirectX::SimpleMath::Vector2::Transform(directionV1, rotationV1) * 50.0f;
	// ベクトルv1を複製する
	DirectX::SimpleMath::Vector2 normalizeV1 = m_vectorV1;
	// ベクトルv1を正規化する
	normalizeV1.Normalize();

	// 向きを設定する
	DirectX::SimpleMath::Vector2 directionV2(1.0f, -1.0f);
	// 回転行列を生成する
	DirectX::SimpleMath::Matrix rotationV2 = DirectX::SimpleMath::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(m_angleV2));
	// ベクトルv2に方向と大きさ(50)を設定する
	m_vectorV2 = DirectX::SimpleMath::Vector2::Transform(directionV2, rotationV2) * 50.0f;
	// 正射影を行う
	m_projection = m_vectorV2.Dot(normalizeV1) * normalizeV1;
	// 視点ベクトルを取得する
	auto eyePosition = m_game->GetCamera()->GetEyePosition();
	// 視点と注視点の距離を計算する
	m_distance = eyePosition.Length();

	// 平面を初期化する
	DirectX::SimpleMath::Plane plane(0.0f, 1.0f, 0.0f, 0.0f);
	// マウスカーソルのスクリーン位置を取得する
	m_mouseCursorPosition = DirectX::SimpleMath::Vector2(roundf((float)m_mouseState.x), roundf((float)m_mouseState.y));
	// カメラをコントロールする
	ControlCamera(timer);
}

// 描画する
void GraphScene::Render()
{
	const DirectX::XMVECTORF32 xaxis = { 100.f, 0.0f, 0.0f };
	const DirectX::XMVECTORF32 yaxis = { 0.0f, 0.0f, 100.f };

	// グリッドを描画する
	DrawGrid(xaxis, yaxis, DirectX::g_XMZero, 10, 10, DirectX::Colors::DarkGray);
	// プリミティブ描画を開始する
	m_graphics->DrawPrimitiveBegin(m_graphics->GetViewMatrix(), m_graphics->GetProjectionMatrix());

	// ベクトルv1を描画する
	m_graphics->DrawVector(DirectX::SimpleMath::Vector2(0.0f, 0.0f), m_vectorV1, DirectX::Colors::White);
	// 白色の円を描画する
	m_graphics->DrawCircle(m_vectorV1, 3.0f, DirectX::Colors::White);
	// ベクトルv2を描画する
	m_graphics->DrawVector(DirectX::SimpleMath::Vector2(0.0f, 0.0f), m_vectorV2, DirectX::Colors::Blue);
	// 青色の円を描画する
	m_graphics->DrawCircle(m_vectorV2, 3.0f, DirectX::Colors::Blue);
	// 正射影ベクトルを描画する
	m_graphics->DrawVector(DirectX::SimpleMath::Vector2(0.0f, 0.0f), m_projection, DirectX::Colors::Red);
	// ベクトルCを描画する
	m_graphics->DrawVector(m_vectorV2, m_projection - m_vectorV2, DirectX::Colors::Yellow);

	// プリミティブ描画を終了する
	m_graphics->DrawPrimitiveEnd();
	// 情報を表示する
	DrawInfo();
}

// 後処理を行う
void GraphScene::Finalize()
{
}

// 情報を表示する
void GraphScene::DrawInfo()
{
	wchar_t stringBuffer[128];
	// SpriteString2Dを宣言する
	SpriteString2D spriteString2D;

	// カメラ位置を書式化する
	swprintf(stringBuffer, sizeof(stringBuffer) / sizeof(wchar_t), L"Camera position: (%6.1f, %6.1f, %6.1f)",
		m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z);
	// カメラ位置を書式化した文字列と表示する座標を追加する
	spriteString2D.AddString(stringBuffer, DirectX::SimpleMath::Vector2(0.0f, 0.0f));
	// カメラ回転角を書式化する
	swprintf(stringBuffer, sizeof(stringBuffer) / sizeof(wchar_t), L"Camera rotation: (%6.1f, %6.1f, %6.1f), %6.1f)",
		m_cameraRotation.x, m_cameraRotation.y, m_cameraRotation.z, m_cameraRotation.w);
	// カメラ回転角を書式化した文字列と表示する座標を追加する
	spriteString2D.AddString(stringBuffer, DirectX::SimpleMath::Vector2(0.0f, 28.0f));
	// ベクトルv1の位置を書式化する
	swprintf(stringBuffer, sizeof(stringBuffer) / sizeof(wchar_t), L"Vector v1 position: (%6.1f, %6.1f)",
		m_vectorV1.x, m_vectorV1.y);
	// ベクトルv1を書式化した文字列と表示する座標を追加する
	spriteString2D.AddString(stringBuffer, DirectX::SimpleMath::Vector2(0.0f, 56.0f));
	// ベクトルv2の位置を書式化する
	swprintf(stringBuffer, sizeof(stringBuffer) / sizeof(wchar_t), L"Vector v2 position: (%6.1f, %6.1f)",
		m_vectorV2.x, m_vectorV2.y);
	// ベクトルv2を書式化した文字列と表示する座標を追加する
	spriteString2D.AddString(stringBuffer, DirectX::SimpleMath::Vector2(0.0f, 84.0f));
	// 正射影ベクトルを書式化する
	swprintf(stringBuffer, sizeof(stringBuffer) / sizeof(wchar_t), L"Projection: (%6.1f, %6.1f)", m_projection.x, m_projection.y);
	// 正射影ベクトルを書式化した文字列と表示する座標を追加する
	spriteString2D.AddString(stringBuffer, DirectX::SimpleMath::Vector2(0.0f, 112.0f));
	// すべての情報を描画する
	spriteString2D.Render();
}

// アークボールを使用してカメラをコントロールする
void GraphScene::ControlCamera(const DX::StepTimer& timer)
{
	// 経過時間を取得する
	float elapsedTime = float(timer.GetElapsedSeconds());
	// スケールゲイン
	constexpr float SCALE_GAIN = 0.001f;

	// カメラ移動行列
	DirectX::SimpleMath::Matrix im;
	m_viewMatrix.Invert(im);
	DirectX::SimpleMath::Vector3 move = DirectX::SimpleMath::Vector3::TransformNormal(move, im);

	// マウスの移動が相対マウス移動である場合
	if (m_game->GetMouse()->GetState().positionMode == DirectX::Mouse::MODE_RELATIVE)
	{
		DirectX::SimpleMath::Vector3 delta = DirectX::SimpleMath::Vector3(-float(m_mouseState.x), float(m_mouseState.y), 0.f) * m_distance;
		delta = DirectX::SimpleMath::Vector3::TransformNormal(delta, im);
		// カメラフォーカス位置を計算する
		m_cameraFocus += delta * elapsedTime;
	}
	// マウスの右ボタンをドラッグしている場合
	else if (m_ballCamera.IsDragging())
	{
		// マウスの移動
		m_ballCamera.OnMove(m_mouseState.x, m_mouseState.y);
		// ボールカメラの現在のクォータニオンを取得する
		auto q = m_ballCamera.GetQuaternion();
		// カメラ回転の逆コォータニオンを計算する
		q.Inverse(m_cameraRotation);
	}
	else
	{
		// マウスフォイールを回転させた場合のズーム値を計算する
		m_zoom = 1.0f + float(m_mouseState.scrollWheelValue) * SCALE_GAIN;
		// ズーム値を調整する
		m_zoom = std::max(m_zoom, 0.01f);
		// スクロールフォイール値をリセットする
		m_game->GetMouse()->ResetScrollWheelValue();
	}

	// ドラッグ中でない場合
	if (!m_ballCamera.IsDragging())
	{
		// マウスの右ボタンを押し下げている場合
		if (m_mouseStateTracker.rightButton == DirectX::Mouse::ButtonStateTracker::PRESSED)
		{
			// 左右の[Ctrlキー]を押し下げている場合
			if (m_keyboardState.LeftControl || m_keyboardState.RightControl)
			{
				// ボールカメラを開始する
				m_ballCamera.OnBegin(m_mouseState.x, m_mouseState.y);
			}
		}
	}
	// マウスの右ボタンを解放している場合
	else if (m_mouseStateTracker.rightButton == DirectX::Mouse::ButtonStateTracker::RELEASED)
	{
		// ボールカメラを終了する
		m_ballCamera.OnEnd();
	}
	// カメラの向きを更新する
	auto direction = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::Backward, m_cameraRotation);
	// カメラ位置を計算する
	m_cameraPosition = m_cameraFocus + (m_distance * m_zoom) * direction;
	// 視線ベクトルを設定する
	m_game->GetCamera()->SetEyePosition(m_cameraPosition);
}


// グリッドを描画する
void GraphScene::DrawGrid(
	const DirectX::FXMVECTOR& xAxis,
	const DirectX::FXMVECTOR& yAxis,
	const DirectX::FXMVECTOR& origin,
	const size_t& xdivs,
	const size_t& ydivs,
	const DirectX::GXMVECTOR& m_color
)
{
	// パフォーマンス開始イベント
	m_graphics->GetDeviceResources()->PIXBeginEvent(L"Draw Grid");
	// プリミティブ描画を開始する
	m_graphics->DrawPrimitiveBegin(m_graphics->GetViewMatrix(), m_graphics->GetProjectionMatrix());

	for (size_t index = 0; index <= xdivs; ++index)
	{
		float percent = float(index) / float(xdivs);
		percent = (percent * 2.0f) - 1.0f;
		// スケールを計算する
		DirectX::XMVECTOR scale = DirectX::XMVectorScale(xAxis, percent);
		scale = DirectX::XMVectorAdd(scale, origin);
		// 頂点1を設定する
		DirectX::VertexPositionColor v1(DirectX::XMVectorSubtract(scale, yAxis), m_color);
		// 頂点2を設定する
		DirectX::VertexPositionColor v2(DirectX::XMVectorAdd(scale, yAxis), m_color);
		// 直線を描画する
		m_graphics->GetPrimitiveBatch()->DrawLine(v1, v2);
	}

	for (size_t index = 0; index <= ydivs; index++)
	{
		float percent = float(index) / float(ydivs);
		percent = (percent * 2.0f) - 1.0f;
		// スケールを計算する
		DirectX::XMVECTOR scale = DirectX::XMVectorScale(yAxis, percent);
		scale = DirectX::XMVectorAdd(scale, origin);
		// 頂点1を設定する
		DirectX::VertexPositionColor v1(DirectX::XMVectorSubtract(scale, xAxis), m_color);
		// 頂点2を設定する
		DirectX::VertexPositionColor v2(DirectX::XMVectorAdd(scale, xAxis), m_color);
		// 直線を描画する
		m_graphics->GetPrimitiveBatch()->DrawLine(v1, v2);
	}
	// プリミティブバッチを終了する
	m_graphics->DrawPrimitiveEnd();
	// パフォーマンス終了イベント
	m_graphics->GetDeviceResources()->PIXEndEvent();
}

// プロジェクションを生成する
void GraphScene::CreateProjection()
{
	// ウィンドウサイズを取得する
	auto size = m_graphics->GetDeviceResources()->GetOutputSize();
	// プロジェクションを生成する
	m_projectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(m_fov, float(size.right) / float(size.bottom), m_nearPlane, m_farPlane);
}

// アークボールのためのウィンドウサイズを設定する
void GraphScene::SetWindow(const int& width, const int& height)
{
	// アークボールのウィンドウサイズを設定する
	m_ballCamera.SetWindow(width, height);
}
