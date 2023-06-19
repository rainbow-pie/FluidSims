#pragma once

#include "glm/glm.hpp"

#include <vector>
#include <memory>

namespace FluidSims
{

	//struct Canvas
	//{
	//	std::size_t width;
	//	std::size_t height;

	//	std::size_t cX(const std::size_t x) const {
	//		return x * cScale;
	//	}

	//	std::size_t cY(const std::size_t y) const {
	//		return canvas.height - y * cScale;
	//	}
	//};

	enum FIELD_TYPE
	{
		H_FIELD = 0,
		V_FIELD = 1,
		S_FIELD = 2,
	};

	std::size_t cnt = 0;

	// ----------------- start of simulator ------------------------------

	float simHeight = 1.1;
	//float cScale = canvas.height / simHeight;
	//float simWidth = canvas.width / cScale;
	float overRelaxation = 1.9;

	glm::vec2 obstacle_pos = { 0.4f, 0.5f };
	float obstacle_speed = 0.0f;

	class Fluid;

	class Integrator
	{
	public:
		virtual void integrate(float& value, float dt, const float gravity) = 0;

	};

	class Fluid {
	public:
		//Canvas canvas;

		float h = 0;
		std::size_t numX = 0;
		std::size_t numY = 0;
		std::size_t numCells = 0;

		float density;

		std::vector<float> h_v;
		std::vector<float> newH_v;
		std::vector<float> v_v;
		std::vector<float> newV_v;
		std::vector<float> pressure;
		std::vector<float> solid;
		std::vector<float> smoke;
		std::vector<float> newSmoke;

		Integrator* integrator = nullptr;

		Fluid(Integrator* integrator, const float density, const std::size_t numX, const std::size_t numY, const float h)
			: integrator(integrator)
		{
			this->density = density;

			this->numX = numX + 2;
			this->numY = numY + 2;
			numCells = this->numX * this->numY;
			this->h = h;

			h_v.resize(numCells);
			newH_v.resize(numCells);
			v_v.resize(numCells);
			newV_v.resize(numCells);
			pressure.resize(numCells, 0.0f);
			solid.resize(numCells, 1.0f);
			smoke.resize(numCells, 1.0f);
			newSmoke.resize(numCells, 0.0f);
		}

		void integrate(float dt, const float gravity)
		{
			std::size_t n = numY;
			for (std::size_t i = 1; i < numX; i++) {
				for (std::size_t j = 1; j < numY - 1; j++) {
					if (solid[i * n + j] != 0.0f && solid[i * n + j - 1] != 0.0f)
						integrator->integrate(v_v[i * n + j], dt, gravity);
				}
			}
		}

		void solveIncompressibility(const std::size_t numIters, const float dt) {

			std::size_t n = this->numY;
			float cp = this->density * this->h / dt;

			for (std::size_t iter = 0; iter < numIters; iter++) {

				for (std::size_t i = 1; i < this->numX - 1; i++) {
					for (std::size_t j = 1; j < this->numY - 1; j++) {

						if (this->solid[i * n + j] == 0.0f)
							continue;

						float solid = this->solid[i * n + j];
						float sx0 = this->solid[(i - 1) * n + j];
						float sx1 = this->solid[(i + 1) * n + j];
						float sy0 = this->solid[i * n + j - 1];
						float sy1 = this->solid[i * n + j + 1];
						solid = sx0 + sx1 + sy0 + sy1;
						if (solid == 0.0f)
							continue;

						const float u1 = this->h_v[(i + 1) * n + j];
						const float u2 = this->h_v[i * n + j];
						const float v1 = this->v_v[i * n + j + 1];
						const float v2 = this->v_v[i * n + j];

						const float div = u1 - u2 + v1 - v2;

						float pressure = -div / solid;
						pressure *= overRelaxation;
						this->pressure[i * n + j] += cp * pressure;

						this->h_v[i * n + j] -= sx0 * pressure;
						this->h_v[(i + 1) * n + j] += sx1 * pressure;
						this->v_v[i * n + j] -= sy0 * pressure;
						this->v_v[i * n + j + 1] += sy1 * pressure;
					}
				}
			}
		}

		void extrapolate() {

			std::size_t n = this->numY;
			for (std::size_t i = 0; i < this->numX; i++) {
				this->h_v[i * n + 0] = this->h_v[i * n + 1];
				this->h_v[i * n + this->numY - 1] = this->h_v[i * n + this->numY - 2];
			}
			for (std::size_t j = 0; j < this->numY; j++) {
				this->v_v[0 * n + j] = this->v_v[1 * n + j];
				this->v_v[(this->numX - 1) * n + j] = this->v_v[(this->numX - 2) * n + j];
			}
		}

		float sampleField(float x, float y, const FIELD_TYPE field) const {
			std::size_t n = this->numY;
			float h = this->h;
			float h1 = 1.0f / h;
			float h2 = 0.5f * h;

			x = std::max(std::min(x, this->numX * h), h);
			y = std::max(std::min(y, this->numY * h), h);

			float dx = 0.0f;
			float dy = 0.0f;

			const std::vector<float>* f = nullptr;

			switch (field) {
			case H_FIELD: { f = &this->h_v; dy = h2; break; }
			case V_FIELD: { f = &this->v_v; dx = h2; break; }
			case S_FIELD: { f = &this->smoke; dx = h2; dy = h2; break; }

			}

			float x0 = std::min(std::floor((x - dx) * h1), static_cast<float>(this->numX - 1));
			float tx = ((x - dx) - x0 * h) * h1;
			float x1 = std::min(x0 + 1, static_cast<float>(this->numX - 1));

			float y0 = std::min(std::floor((y - dy) * h1), static_cast<float>(this->numY - 1));
			float ty = ((y - dy) - y0 * h) * h1;
			float y1 = std::min(y0 + 1, static_cast<float>(this->numY - 1));

			float sx = 1.0f - tx;
			float sy = 1.0f - ty;

			float val = sx * sy * (*f)[x0 * n + y0] +
				tx * sy * (*f)[x1 * n + y0] +
				tx * ty * (*f)[x1 * n + y1] +
				sx * ty * (*f)[x0 * n + y1];

			return val;
		}

		float avgH(const std::size_t i, const std::size_t j) {
			std::size_t n = this->numY;
			float h_v = (this->h_v[i * n + j - 1] + this->h_v[i * n + j] +
				this->h_v[(i + 1) * n + j - 1] + this->h_v[(i + 1) * n + j]) * 0.25f;
			return h_v;

		}

		float avgV(const std::size_t i, const std::size_t j) {
			std::size_t n = this->numY;
			float v_v = (this->v_v[(i - 1) * n + j] + this->v_v[i * n + j] +
				this->v_v[(i - 1) * n + j + 1] + this->v_v[i * n + j + 1]) * 0.25f;
			return v_v;
		}

		void advectVel(const float dt) {

			this->newH_v = this->h_v;
			this->newV_v = this->v_v;

			std::size_t n = this->numY;
			float h = this->h;
			float h2 = 0.5f * h;

			std::size_t cnt = 0;

			for (std::size_t i = 1; i < this->numX; i++) {
				for (std::size_t j = 1; j < this->numY; j++) {

					cnt++;

					// h_v component
					if (this->solid[i * n + j] != 0.0f && this->solid[(i - 1) * n + j] != 0.0f && j < this->numY - 1) {
						float x = i * h;
						float y = j * h + h2;
						float h_v = this->h_v[i * n + j];
						float v_v = this->avgV(i, j);
						//float v_v = this->sampleField(x, y, V_FIELD);
						x = x - dt * h_v;
						y = y - dt * v_v;
						h_v = this->sampleField(x, y, H_FIELD);
						this->newH_v[i * n + j] = h_v;
					}
					// v_v component
					if (this->solid[i * n + j] != 0.0f && this->solid[i * n + j - 1] != 0.0f && i < this->numX - 1) {
						float x = i * h + h2;
						float y = j * h;
						float h_v = this->avgH(i, j);
						//float h_v = this->sampleField(x, y, U_FIELD);
						float v_v = this->v_v[i * n + j];
						x = x - dt * h_v;
						y = y - dt * v_v;
						v_v = this->sampleField(x, y, V_FIELD);
						this->newV_v[i * n + j] = v_v;
					}
				}
			}

			this->h_v = this->newH_v;
			this->v_v = this->newV_v;
		}

		void advectSmoke(const float dt)
		{

			this->newSmoke = this->smoke;

			std::size_t n = this->numY;
			float h = this->h;
			float h2 = 0.5f * h;

			for (std::size_t i = 1; i < this->numX - 1; i++) {
				for (std::size_t j = 1; j < this->numY - 1; j++) {

					if (this->solid[i * n + j] != 0.0f) {
						float h_v = (this->h_v[i * n + j] + this->h_v[(i + 1) * n + j]) * 0.5f;
						float v_v = (this->v_v[i * n + j] + this->v_v[i * n + j + 1]) * 0.5f;
						float x = i * h + h2 - dt * h_v;
						float y = j * h + h2 - dt * v_v;

						this->newSmoke[i * n + j] = this->sampleField(x, y, S_FIELD);
					}
				}
			}
			this->smoke = this->newSmoke;
		}

		// ----------------- end of simulator ------------------------------


		void simulate(const float dt, const float gravity, const std::size_t numIters) {

			this->integrate(dt, gravity);

			pressure.resize(this->numCells, 0.0f);
			for (float& pressure : pressure)
			{
				pressure = 0.0f;
			}
			this->solveIncompressibility(numIters, dt);

			this->extrapolate();
			this->advectVel(dt);
			this->advectSmoke(dt);
		}

		//RigidBody obstacle{ {0.5f, 0.5f}, 5.0f };
		// 
		//var scene =
		//{
		//  float gravity = -9.81f,
		//  float dt = 1.0f / 120.0f,
		//  float numIters = 100f,
		//  frameNr = 0,
		//  float overRelaxation = 1.9f,
		//  float obstacleX = 0.0f,
		//  float obstacleY = 0.0f,
		//  float obstacleRadius = 0.15f,
		//  bool paused = false,
		//  sceneNr = 0,
		//  bool showObstacle = false,
		//  bool showStreamlines = false,
		//  bool showVelocities = false,
		//  bool showPressure = false,
		//  bool showSmoke = true,
		//  fluid = null
		//};
	};


	class IntegratorEuler : public Integrator
	{
	public:

		void integrate(float& value, float dt, const float gravity) override
		{
			value += gravity * dt;
		}
	};


	struct RigidBody
	{
		enum type_t
		{
			none,
			circle,
			square,
			triangle
		};

		type_t type;
		glm::vec2 pos;
		glm::vec2 speed;
		float radius;
		glm::vec2 size;
	};

	enum class scene_type_t
	{
		wind_tunnel,
		paint
	};

}


template<typename T>
T map_in_range(const T x, const T in_min, const T in_max, const T out_min, const T out_max)
{
	T result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

	if (result > out_max)
		result = out_max;
	else if (result < out_min)
		result = out_min;

	return result;
}

glm::vec3 getSciColor(float val, float minVal, float maxVal) {
	val = std::min(std::max(val, minVal), maxVal - 0.0001f);
	float d = maxVal - minVal;
	val = d == 0.0f ? 0.5f : (val - minVal) / d;
	float smoke = 0.25f;
	std::size_t num = std::floor(val / smoke);
	float solid = (val - num * smoke) / smoke;
	float r, g, b;

	switch (num) {
	case 0: r = 0.0; g = solid; b = 1.0; break;
	case 1: r = 0.0; g = 1.0; b = 1.0 - solid; break;
	case 2: r = solid; g = 1.0; b = 0.0; break;
	case 3: r = 1.0; g = 1.0 - solid; b = 0.0; break;
	default: return { 0.5f, 0.5f, 0.0f }; break;
	}

	return { r, g, b };
}


void draw_streamlines_to_vector(const FluidSims::Fluid& fluid, std::vector<float>& lines, const std::size_t num_segs, const float seg_length)
{
	auto add_point_lines = [&lines](const glm::vec3& location, const glm::vec3& color)
	{
		lines.push_back(location.x);
		lines.push_back(location.y);
		lines.push_back(location.z);

		lines.push_back(color[0]);
		lines.push_back(color[1]);
		lines.push_back(color[2]);
	};

	//const float segLength = 1.0f;

	for (std::size_t i = 1; i < fluid.numX - 1; i += 5) {
		for (std::size_t j = 1; j < fluid.numY - 1; j += 5) {

			float x = (i + 0.5f);
			float y = (j + 0.5f);

			for (std::size_t n = 0; n < num_segs; ++n)
			{
				add_point_lines({ x - fluid.numX / 2 - 1, y - fluid.numY / 2 - 10, -65.0f }, { 0.1f, 0.1f, 0.1f });
				const float h_v = fluid.sampleField(x * fluid.h, y * fluid.h, FluidSims::H_FIELD);
				const float v_v = fluid.sampleField(x * fluid.h, y * fluid.h, FluidSims::V_FIELD);
				//const float l = std::sqrt(h_v * h_v + v_v * v_v);
				 //x += h_v / l * segLength;
				 //y += v_v / l * segLength;
				x += h_v * seg_length / fluid.h;
				y += v_v * seg_length / fluid.h;


				add_point_lines({ x - fluid.numX / 2 - 1, y - fluid.numY / 2 - 10, -65.0f }, { 0.1f, 0.1f, 0.1f });

				if (x > fluid.numX)
					break;
			}

		}
	}
}