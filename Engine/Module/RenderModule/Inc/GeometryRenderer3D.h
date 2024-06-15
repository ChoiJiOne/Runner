#pragma once

#include <array>

#include "RenderModule.h"
#include "VertexBuffer.h"

class Shader;


/**
 * @brief 3D 기하 도형을 렌더링하는 렌더러입니다.
 */
class GeometryRenderer3D : public IResource
{
public:
	/**
	 * @brief 3D 기하 도형을 그리는 렌더러의 디폴트 생성자입니다.
	 */
	explicit GeometryRenderer3D();


	/**
	 * @brief 3D 기하 도형을 그리는 렌더러의 가상 소멸자입니다.
	 *
	 * @note 렌더러 내부에서 할당된 요소를 해제하기 위해서는 반드시 Release를 호출해야 합니다.
	 */
	virtual ~GeometryRenderer3D();


	/**
	 * @brief 3D 기하 도형을 그리는 렌더러의 복사 생성자와 대입 연산자를 명시적으로 삭제합니다.
	 */
	DISALLOW_COPY_AND_ASSIGN(GeometryRenderer3D);


	/**
	 * @brief 렌더러 내부 리소스를 할당 해제합니다.
	 */
	virtual void Release() override;


	/**
	 * @brief 렌더러의 파라미터를 설정합니다.
	 *
	 * @param view 설정할 뷰 행렬입니다.
	 */
	void SetView(const Mat4x4& view) { view_ = view; }


	/**
	 * @brief 렌더러의 원근 투영 행렬을 설정합니다.
	 *
	 * @param projection 설정할 투영 행렬입니다.
	 */
	void SetProjection(const Mat4x4& projection) { projection_ = projection; }


	/**
	 * @brief 3D 점들을 그립니다.
	 *
	 * @param positions 화면 상의 3D 점들입니다.
	 * @param color 점들의 색상입니다.
	 * @param pointSize 점의 크기입니다. 기본 값은 1.0f입니다.
	 *
	 * @note 3D 점들의 개수는 MAX_VERTEX_SIZE(10000)의 크기를 넘을 수 없습니다.
	 */
	void DrawPoints3D(const std::vector<Vec3f>& positions, const Vec4f& color, float pointSize = 1.0f);


	/**
	 * @brief 점들을 연결한 3D 선을 그립니다.
	 *
	 * @param positions 화면 상의 3D 점들입니다.
	 * @param color 점들의 색상입니다.
	 *
	 * @note 2D 점들의 개수는 MAX_VERTEX_SIZE(10000)의 크기를 넘을 수 없습니다.
	 */
	void DrawConnectPoints3D(const std::vector<Vec3f>& positions, const Vec4f& color);


	/**
	 * @brief 3D 선을 그립니다.
	 *
	 * @param fromPosition 선의 시작점입니다.
	 * @param toPosition 선의 끝점입니다.
	 * @param color 선의 색상입니다.
	 */
	void DrawLine3D(const Vec3f& fromPosition, const Vec3f& toPosition, const Vec4f& color);


	/**
	 * @brief 3D 선을 그립니다.
	 *
	 * @param fromPosition 선의 시작점입니다.
	 * @param fromColor 선의 시작점 색상입니다.
	 * @param toPosition 선의 끝점입니다.
	 * @param toColor 선의 끝점 색상입니다.
	 */
	void DrawLine3D(const Vec3f& fromPosition, const Vec4f& fromColor, const Vec3f& toPosition, const Vec4f& toColor);


	/**
	 * @brief 3D 선들을 그립니다.
	 *
	 * @param positions 연결되지 않은 선의 점 목록입니다.
	 * @param color 선의 색상입니다.
	 */
	void DrawLines3D(const std::vector<Vec3f>& positions, const Vec4f& color);


	/**
	 * @brief 3D 쿼드를 그립니다.
	 *
	 * @param world 월드 행렬입니다.
	 * @param width 쿼드의 가로 크기입니다.
	 * @param height 쿼드의 세로 크기입니다.
	 * @param color 쿼드의 색상입니다.
	 *
	 * @note 3D 쿼드는 XY평면 기준입니다.
	 */
	void DrawQuad3D(const Mat4x4& world, float width, float height, const Vec4f& color);


	/**
	 * @brief 화면에 분할된 3D 쿼드를 그립니다.
	 *
	 * @param world 월드 행렬입니다.
	 * @param view 시야 행렬입니다.
	 * @param projection 투영 행렬입니다.
	 * @param width 쿼드의 가로 크기입니다.
	 * @param height 쿼드의 세로 크기입니다.
	 * @param rate 쿼드의 분할 비율입니다. 값의 범위는 0.0f ~ 1.0f입니다.
	 * @param color 쿼드의 rate 비율 부분의 색상입니다.
	 * @param bgColor 쿼드의 1.0f - rate 비율 부분의 색상입니다.
	 *
	 * @note
	 * - 3D 쿼드는 XY 평면 기준입니다.
	 * - 분할 기준입니다.
	 * ┌────────┬────────────────┐
	 * │        │                │
	 * │  rate  │  1.0f - rate   │
	 * │        │                │
	 * └────────┴────────────────┘
	 */
	void DrawHorizonProgressBar3D(const Mat4x4& world, float width, float height, float rate, const Vec4f& color, const Vec4f& bgColor);


	/**
	 * @brief 화면에 분할된 3D 쿼드를 그립니다.
	 *
	 * @param world 월드 행렬입니다.
	 * @param width 쿼드의 가로 크기입니다.
	 * @param height 쿼드의 세로 크기입니다.
	 * @param rate 쿼드의 분할 비율입니다. 값의 범위는 0.0f ~ 1.0f입니다.
	 * @param color 쿼드의 rate 비율 부분의 색상입니다.
	 * @param bgColor 쿼드의 1.0f - rate 비율 부분의 색상입니다.
	 *
	 * @note
	 * - 3D 쿼드는 XY 평면 기준입니다.
	 * - 분할 기준입니다.
	 * ┌──────────────┐
	 * │ 1.0f - rate  │
	 * ├──────────────┤
	 * │     rate     │
	 * └──────────────┘
	 */
	void DrawVerticalProgressBar3D(const Mat4x4& world, float width, float height, float rate, const Vec4f& color, const Vec4f& bgColor);


	/**
	 * @brief 3D 큐브를 그립니다.
	 *
	 * @param world 월드 행렬입니다.
	 * @param extents 큐브의 X/Y/Z 축 방향으로의 크기입니다.
	 * @param color 큐브의 색상입니다.
	 */
	void DrawCube3D(const Mat4x4& world, const Vec3f& extents, const Vec4f& color);


	/**
	 * @brief 3D 구를 그립니다.
	 *
	 * @param world 월드 행렬입니다.
	 * @param radius 구의 반지름 길이입니다.
	 * @param color 구의 색상입니다.
	 */
	void DrawSphere3D(const Mat4x4& world, float radius, const Vec4f& color);


	/**
	 * @brief 3D 시야 절두체를 그립니다.
	 * 
	 * @param view 시야 절두체의 시야 행렬입니다.
	 * @param projection 시야 절두체의 투영 행렬입니다.
	 * @param color 시야 절두체의 색상입니다.
	 */
	void DrawViewfrustum3D(const Mat4x4& view, const Mat4x4& projection, const Vec4f& color);


	/**
	 * @brief 격자를 그립니다.
	 *
	 * @param extensions 격자의 XYZ 순의 크기입니다.
	 * @param stride 격자의 간격입니다.
	 */
	void DrawGrid3D(const Vec3f& extensions, float stride);


private:
	/**
	 * @brief 3D 기하 도형을 렌더링하는 렌더러 내부에서 사용하는 정점입니다.
	 */
	struct Vertex
	{
		/**
		 * @brief 정점의 기본 생성자입니다.
		 */
		Vertex() noexcept
			: position(0.0f, 0.0f, 0.0f)
			, color(0.0f, 0.0f, 0.0f, 0.0f) {}


		/**
		 * @brief 정점의 생성자입니다.
		 *
		 * @param p 정점의 위치입니다.
		 * @param c 정점의 색상입니다.
		 */
		Vertex(Vec3f&& p, Vec4f&& c) noexcept
			: position(p)
			, color(c) {}


		/**
		 * @brief 정점의 생성자입니다.
		 *
		 * @param p 정점의 위치입니다.
		 * @param c 정점의 색상입니다.
		 */
		Vertex(const Vec3f& p, const Vec4f& c) noexcept
			: position(p)
			, color(c) {}


		/**
		 * @brief 정점의 생성자입니다.
		 *
		 * @param x 위치의 X좌표입니다.
		 * @param y 위치의 Y좌표입니다.
		 * @param z 위치의 Z좌표입니다.
		 * @param r 정점 색상의 R입니다.
		 * @param g 정점 색상의 G입니다.
		 * @param b 정점 색상의 B입니다.
		 * @param a 정점 색상의 A입니다.
		 */
		Vertex(
			float x, float y, float z,
			float r, float g, float b, float a
		) noexcept
			: position(x, y, z)
			, color(r, g, b, a) {}


		/**
		 * @brief 정점의 생성자입니다.
		 *
		 * @param instance 복사할 정점 인스턴스입니다.
		 */
		Vertex(Vertex&& instance) noexcept
			: position(instance.position)
			, color(instance.color) {}


		/**
		 * @brief 정점의 생성자입니다.
		 *
		 * @param instance 복사할 정점 인스턴스입니다.
		 */
		Vertex(const Vertex& instance) noexcept
			: position(instance.position)
			, color(instance.color) {}


		/**
		 * @brief 정점의 대입 연산자입니다.
		 *
		 * @param instance 대입할 정점 인스턴스입니다.
		 *
		 * @return 대입한 객체의 참조자를 반환합니다.
		 */
		Vertex& operator=(Vertex&& instance) noexcept
		{
			if (this == &instance) return *this;

			position = instance.position;
			color = instance.color;

			return *this;
		}


		/**
		 * @brief 정점의 대입 연산자입니다.
		 *
		 * @param instance 대입할 정점 인스턴스입니다.
		 *
		 * @return 대입한 객체의 참조자를 반환합니다.
		 */
		Vertex& operator=(const Vertex& instance) noexcept
		{
			if (this == &instance) return *this;

			position = instance.position;
			color = instance.color;

			return *this;
		}


		/**
		 * @brief 정점의 바이트 보폭 값을 얻습니다.
		 *
		 * @return 정점의 파이트 보폭(Stride) 값을 반환합니다.
		 */
		static uint32_t GetStride()
		{
			return sizeof(Vertex);
		}


		/**
		 * @brief 정점의 위치입니다.
		 */
		Vec3f position;


		/**
		 * @brief 정점의 색상입니다.
		 */
		Vec4f color;
	};


private:
	/**
	 * @brief 기하 도형 그리기를 수행합니다.
	 *
	 * @parma world 월드 행렬입니다.
	 * @param drawMode 그리기 타입입니다.
	 * @param vertexCount 정점 수입니다.
	 */
	void DrawGeometry3D(const Mat4x4& world, const EDrawMode& drawMode, uint32_t vertexCount);


private:
	/**
	 * @brief 도형 정점 목록의 최대 크기입니다.
	 */
	static const int32_t MAX_VERTEX_SIZE = 10000;


	/**
	 * @brief 곡선 계산 시 최대 분할 횟수입니다.
	 */
	static const int32_t MAX_SLICE_SIZE = 7;


	/**
	 * @brief 도형의 정점 목록입니다.
	 */
	std::array<Vertex, MAX_VERTEX_SIZE> vertices_;


	/**
	 * @brief 렌더러의 뷰 행렬입니다.
	 */
	Mat4x4 view_;


	/**
	 * @brief 렌더러의 원근 투영 행렬입니다.
	 */
	Mat4x4 projection_;


	/**
	 * @brief 점의 크기입니다.
	 */
	float pointSize_ = 1.0f;


	/**
	 * @brief 정점 버퍼 목록의 오브젝트입니다.
	 */
	uint32_t vertexArrayObject_ = 0;


	/**
	 * @brief 정점 버퍼입니다.
	 */
	VertexBuffer* vertexBuffer_;


	/**
	 * @brief 렌더링 시 사용할 셰이더입니다.
	 */
	Shader* shader_ = nullptr;
};