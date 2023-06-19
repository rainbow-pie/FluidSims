#include "gl/glew.h"
#include "glfw/glfw3.h"

#include "core/Application.h"

#include "fluid_sims.h"

#include <chrono>
#include <iostream>
#include <functional>

#include <vector>
#include <iterator>


struct Scene
{
	bool drawPressure = false;
	bool drawSmoke = true;
	bool drawStreamlines = false;

	std::size_t iterations = 0;

	float overRelaxation = 1.0f;

	float dt = 1.0f / 60;

	glm::vec2 gravity = { 0.0f, 0.0f };

	FluidSims::scene_type_t scene_type;

	std::size_t frameCount = 0;

	std::shared_ptr<FluidSims::Fluid> fluid = nullptr;

	FluidSims::RigidBody obstacle{ FluidSims::RigidBody::none, { 0.0f, 0.0f}, { 0.0f, 0.0f }, 1.0f, { 10.0f, 10.0f} };

	glm::vec2 obstacle_new_pos{ 0.0f, 0.0f };
	FluidSims::RigidBody::type_t obstacle_new_type = FluidSims::RigidBody::none;


	void setObstacleNone()
	{
		const std::size_t n = fluid->numY;

		for (std::size_t i = 1; i < fluid->numX - 2; i++) {
			for (std::size_t j = 1; j < fluid->numY - 2; j++) {

				fluid->solid[i * n + j] = 1.0f;
			}
		}
	}

	void setObstacleCircle(float x, float y, bool reset)
	{
		float vx = 0.0f;
		float vy = 0.0f;

		if (!reset) {
			vx = (x - obstacle.pos.x) / dt;
			vy = (y - obstacle.pos.y) / dt;
		}

		obstacle.pos.x = x;
		obstacle.pos.y = y;
		float r = obstacle.radius;
		std::size_t n = fluid->numY;

		for (std::size_t i = 1; i < fluid->numX - 2; i++) {
			for (std::size_t j = 1; j < fluid->numY - 2; j++) {

				fluid->solid[i * n + j] = 1.0f;

				float dx = (i + 0.5f) - x * fluid->numX;
				float dy = (j + 0.5f) - y * fluid->numY;

				if (dx * dx + dy * dy < r * r) {
					fluid->solid[i * n + j] = 0.0f;;
					if (scene_type == FluidSims::scene_type_t::paint)
						fluid->smoke[i * n + j] = 0.5f + 0.5f * std::sin(0.1f * frameCount);
					else
						fluid->smoke[i * n + j] = 1.0f;

					fluid->h_v[i * n + j] = vx;
					fluid->h_v[(i + 1) * n + j] = vx;
					fluid->v_v[i * n + j] = vy;
					fluid->v_v[i * n + j + 1] = vy;
				}
			}
		}
	}

	void setObstacleSquare(float x, float y, bool reset)
	{
		float vx = 0.0f;
		float vy = 0.0f;

		if (!reset) {
			vx = (x - obstacle.pos.x) / dt;
			vy = (y - obstacle.pos.y) / dt;
		}

		obstacle.pos.x = x;
		obstacle.pos.y = y;

		std::size_t n = fluid->numY;

		for (std::size_t i = 1; i < fluid->numX - 2; i++) {
			for (std::size_t j = 1; j < fluid->numY - 2; j++) {

				fluid->solid[i * n + j] = 1.0f;

				const glm::vec2 point_pos{ i,j };

				const float diff_x = std::abs(point_pos.x - obstacle.pos.x * fluid->numX);
				const float diff_y = std::abs(point_pos.y - obstacle.pos.y * fluid->numY);

				if (
					diff_x <= obstacle.size.x &&
					diff_y <= obstacle.size.y)
				{
					fluid->solid[i * n + j] = 0.0f;
					if (scene_type == FluidSims::scene_type_t::paint)
						fluid->smoke[i * n + j] = 0.5f + 0.5f * std::sin(0.1f * frameCount);
					else
						fluid->smoke[i * n + j] = 1.0f;

					fluid->h_v[i * n + j] = vx;
					fluid->h_v[(i + 1) * n + j] = vx;
					fluid->v_v[i * n + j] = vy;
					fluid->v_v[i * n + j + 1] = vy;
				}
			}
		}
	}

	void setObstacle(float x, float y, bool reset) {

		if (obstacle.type == FluidSims::RigidBody::circle)
			setObstacleCircle(x, y, reset);
		else if (obstacle.type == FluidSims::RigidBody::square)
			setObstacleSquare(x, y, reset);
		else if (obstacle.type == FluidSims::RigidBody::none)
			setObstacleNone();
		else
			assert(false && "Shape not implemented yet");
	}

	void clear_field()
	{
		std::size_t n = fluid->numY;

		for (std::size_t i = 0; i < fluid->numX; i++) {
			for (std::size_t j = 0; j < fluid->numY; j++) {

				fluid->solid[i * n + j] = 1.0f;

				fluid->h_v[i * n + j] = 0.0f;
				fluid->v_v[i * n + j] = 0.0f;

				fluid->smoke[i * n + j] = 1.0f;

				fluid->pressure[i * n + j] = 0.0f;

				if (i == 0 || j == 0 || i == fluid->numX - 1 || j == fluid->numY - 1)
					fluid->solid[i * n + j] = 0.0f;

			}
		}
	}

	void setup_wind_tunnel(const FluidSims::RigidBody::type_t obstacle_type)
	{
		dt = 1.0f / 60;
		overRelaxation = 1.9f;
		gravity = { 0.0f, 0.0f };
		iterations = 100;

		drawPressure = false;
		drawSmoke = true;
		drawStreamlines = false;


		std::size_t n = fluid->numY;

		float inVel = 2.0f;
		for (std::size_t i = 0; i < fluid->numX; i++) {
			for (std::size_t j = 0; j < fluid->numY; j++) {
				float solid = 1.0f;	// fluid
				if (i == 0 || j == 0 || j == fluid->numY - 1)
					solid = 0.0;	// solid
				fluid->solid[i * n + j] = solid;

				if (i == 1) {
					fluid->h_v[i * n + j] = inVel;
				}
				else {
					fluid->h_v[i * n + j] = 0.0f;
				}

				fluid->smoke[i * n + j] = 1.0f;
			}
		}

		float pipeH = 0.1f * fluid->numY;
		std::size_t minJ = std::floor(0.5f * fluid->numY - 0.5f * pipeH);
		std::size_t maxJ = std::floor(0.5f * fluid->numY + 0.5f * pipeH);

		for (std::size_t j = minJ; j < maxJ; j++)
			fluid->smoke[0 * n + j] = 0.0f;

		obstacle.radius = 15.0f;
		obstacle.speed = { 0.0f, 0.0f };
		obstacle.size = { 12.0f, 12.0f };
		obstacle.type = obstacle_type;

		setObstacle(0.4f, 0.5f, true);
	}

	void setup_paint(const FluidSims::RigidBody::type_t obstacle_type)
	{
		dt = 1.0f / 60;
		overRelaxation = 1.0f;
		gravity = { 0.0f, 0.0f };
		iterations = 40;

		drawPressure = false;
		drawSmoke = true;
		drawStreamlines = false;

		obstacle.radius = 15.0f;
		obstacle.speed = { 0.0f, 0.0f };
		obstacle.size = { 12.0f, 12.0f };
		obstacle.type = obstacle_type;

		setObstacle(0.4f, 0.5f, true);
	}

	void setup_scene(const FluidSims::scene_type_t type, const FluidSims::RigidBody::type_t obstacle_type)
	{
		scene_type = type;

		clear_field();

		obstacle_new_type = obstacle_type;

		if (type == FluidSims::scene_type_t::wind_tunnel)
			setup_wind_tunnel(obstacle_type);

		else if (type == FluidSims::scene_type_t::paint)
			setup_paint(obstacle_type);

		//for (std::size_t j = minJ; j < maxJ; j++)
		//	fluid.solid[fluid.numX / 2 * n + j] = 0.0f;

		//for (std::size_t j = fluid.numX / 2; j < fluid.numX / 2 + 20; j++)
		//{
		//	fluid.solid[(j)*n + minJ] = 0.0f;
		//	fluid.solid[(j)*n + maxJ] = 0.0f;
		//}
		//glm::vec2 circle_pos = { fluid.numX / 4 - 1, fluid.numY / 2 - 0};
		//float circle_r = fluid.numY * 0.15 / 2 + 0.5f;

		//for (std::size_t i = 0; i < fluid.numX; i++)
		//{
		//	for (std::size_t j = 0; j < fluid.numY; j++)
		//	{
		//		glm::vec2 point_pos(i, j);

		//		if (glm::length(circle_pos - point_pos) < circle_r)
		//			fluid.solid[i * n + j] = 0.0f;
		//	}
		//}

	}

	void processInput(GLFWwindow* window)
	{
		obstacle_new_pos = obstacle.pos;
		//cameraSpeed *= 1.01f;
		//if (cameraSpeed > maxCameraSpeed)
			//cameraSpeed = maxCameraSpeed;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE &&
			glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE &&
			glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
		{
			obstacle.speed = { 0.0f, 0.0f };
		}

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			obstacle.speed.y += 0.0005f;
			obstacle_new_pos.y += obstacle.speed.y;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			obstacle.speed.y += 0.0005f;
			obstacle_new_pos.y -= obstacle.speed.y;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			obstacle.speed.x += 0.0005f;
			obstacle_new_pos.x -= obstacle.speed.x;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			obstacle.speed.x += 0.0005f;
			obstacle_new_pos.x += obstacle.speed.x;
		}
	}

};


class NewGame : public ge::NewScene
{
public:
  //NewGame(ge::SmartPtr<entt::registry> registry)
    //: ge::NewScene(std::move(registry))
  //{
  //}

  entt::entity create_triangle(const glm::vec2& pos, const glm::vec3& color, const float scale)
  {
    entt::entity triangle_entity = ge::EntityManager::get().create_entity();
    
    //TODO: move to special function std::vector<float> create_triangle() in game engine
    std::vector<float> vertices = {
      pos.x - 0.5f * scale, pos.y - 0.5f * scale, 0.0f, color.r, color.g, color.b,
      pos.x + 0.5f * scale, pos.y - 0.5f * scale, 0.0f, color.r, color.g, color.b,
      pos.x + 0.0f * scale, pos.y + 0.5f * scale, 0.0f, color.r, color.g, color.b
    };

    ge::NewDrawable triangle;
    triangle.vertices = std::move(vertices);
    triangle.m_count = 3;

    registry->emplace<ge::NewDrawable>(triangle_entity, std::move(triangle));

    return triangle_entity;
  }

  entt::entity create_square(const glm::vec2& pos, const glm::vec3& color, const float scale)
  {
    entt::entity square_entity = ge::EntityManager::get().create_entity();

    //TODO: move to special function std::vector<float> create_square() in game engine
    std::vector<float> vertices = {
      pos.x - 0.5f * scale, pos.y - 0.5f * scale, color.r, color.g, color.b,
      pos.x + 0.5f * scale, pos.y - 0.5f * scale, color.r, color.g, color.b,
      pos.x + 0.5f * scale, pos.y + 0.5f * scale, color.r, color.g, color.b,

      pos.x + 0.5f * scale, pos.y + 0.5f * scale, color.r, color.g, color.b,
      pos.x - 0.5f * scale, pos.y + 0.5f * scale, color.r, color.g, color.b,
      pos.x - 0.5f * scale, pos.y - 0.5f * scale, color.r, color.g, color.b
    };
    ge::NewDrawable square;
    square.vertices = std::move(vertices);
    square.m_count = 6;

    registry->emplace<ge::NewDrawable>(square_entity, std::move(square));

    return square_entity;
  }

  entt::entity create_player()
  {
    entt::entity player = registry->create();

    ge::SpriteComponent sprite;
    sprite.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\heart.png");

    registry->emplace<ge::SpriteComponent>(player, std::move(sprite));

    ge::TransformComponent transform;
    transform.translation = { -100.0f, 0.0f, 0.0f };
    transform.rotation_angle = 0.0f;
    transform.rotation_axes = { 0.0f, 0.0f, 1.0f };
    transform.scale = { 50.0f, 50.0f, 1.0f };

    registry->emplace<ge::TransformComponent>(player, transform);
    

    entt::entity player2 = registry->create();

    ge::SpriteComponent sprite2;
    sprite2.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\heart.png");

    registry->emplace<ge::SpriteComponent>(player2, std::move(sprite2));

    transform.translation = { 100.0f, 0.0f, 0.0f };
    transform.rotation_angle = 0.0f;
    transform.rotation_axes = { 0.0f, 0.0f, 1.0f };
    transform.scale = { 50.0f, 50.0f, 1.0f };

    registry->emplace<ge::TransformComponent>(player2, transform);

    return player;
  }

  void create_field()
  {

  }

  void on_create() override
  {
    entt::entity player = create_player();

    camera.pos.z = 10.0f;
    camera.center = glm::vec3{ 0.0f, 0.0f, -1.0f };
    camera.up = glm::vec3{ 0.0f, 1.0f, 0.0f };

    camera.pos.x = 0.0f;
    camera.pos.y = 0.0f;

    entt::entity triangl = create_triangle({ 20.0f, 20.0f }, { 0.0f, 1.0f, 0.0f }, 50.0f);
    //entt::entity square1 = create_square({ 10.0f, 7.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
    //entt::entity square2 = create_square({ 7.0f, 5.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
    //entt::entity square3 = create_square({ 5.0f, 2.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);

    //entt::entity player = registry->create();

    //registry->emplace<int>(player, 5);

  };

  void on_update(const double dt) override
  {

  };

};

//kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;odbc32.lib;odbccp32.lib;%(AdditionalDependencies)
int main(int argc, const char** argv)
{

  if (ge::WindowManager::get().init_subsystem() != 0) {
    std::cout << "Something happened while initializing window subsystem\n";
    return -1;
  }
  std::shared_ptr<ge::Window> main_window;
  int width = 840;
  int height = 480;
  if (!(main_window = ge::WindowManager::get().create_window(width, height))) {
    std::cout << "Something happened while creating window\n";
    return -2;
  }

  if (ge::Graphics::get().init_subsystem() != 0) {
    std::cout << "Something happened while initializing graphic subsystem\n";
    return -3;
  }

  ge::Application app;
  app.set_window(main_window);

  if (app.launch() != 0) {
    std::cout << "Something happened while setting up the application\n";
    return -3;
  }

  std::shared_ptr<NewGame> scene1 = std::make_shared<NewGame>();
  app.set_scene(scene1);

  return app.main_loop();
}


class Game : public ge::Scene
{
public:

  Game()
  {
    //ge::DrawablePtr tri = ge::ResourceManager::get().create_object<ge::Triangle>();
    //tri->set({ 2.0f, 2.0f }, { 0.0f, 1.0f, 0.0f });
    //m_objects.push_back(tri);

    //player = tri;

    //ge::DrawablePtr square1 = ge::ResourceManager::get().create_object<ge::Square>();
    //square1->set({ 10.0f, 7.0f }, { 1.0f, 0.0f, 0.0f });
    //m_objects.push_back(square1);

    //ge::DrawablePtr square2 = ge::ResourceManager::get().create_object<ge::Square>();
    //square2->set({ 7.0f, 5.0f }, { 1.0f, 0.0f, 0.0f });
    //m_objects.push_back(square2);


    //ge::DrawablePtr square3 = ge::ResourceManager::get().create_object<ge::Square>();
    //square3->set({ 5.0f, 2.0f }, { 1.0f, 0.0f, 0.0f });
    //m_objects.push_back(square3);

    //ge::drawable_ptr fluid_field = ge::ResourceManager::get().create_object<FluidSims::Field>();
    //ge::SmartPtr<ge::batch> batch;
    //batches



    //m_objects.reserve(100 * 100);

    camera.pos.z = 10.0f;
    camera.center = glm::vec3{ 0.0f, 0.0f, -1.0f };
    camera.up = glm::vec3{ 0.0f, 1.0f, 0.0f };

    for (std::size_t i = 0; i < 50; ++i) {
      ge::DrawablePtr square_new = std::make_shared<ge::Square>();
      square_new->set({ 1.1f * i - 5, 0.2f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
      //m_objects.push_back(square_new);
    }

    //for (std::size_t i = 0; i < 10; ++i) {
    //  for (std::size_t j = 0; j < 50; ++j) {
    //    ge::drawable_ptr square_new = std::make_shared<ge::Square>();
    //    square_new->set({ 0.2f * i, 0.2f * j }, { 1.0f, 0.0f, 0.0f }, 0.1f);
    //    m_objects.push_back(square_new);
    //  }
    //}
  }
  void on_update(double dt) override
  {
    static ge::Animation<float> anim(0.0f, 5.0f, 5.0f);

    const double gravity = -9.7 / 1;
    player->move({ player_speed.x * dt, player_speed.y * dt });
    player_speed.y += gravity * dt;

    camera.pos.x = player->pos().x;
    camera.pos.y = player->pos().y;
  }

  void on_collision(ge::DrawablePtr first, ge::DrawablePtr second, glm::vec2 mpv) override
  {
    static std::size_t counter = 0;
    if (first.get() && second.get()) {
      if (second == player)
      {
        second->move(mpv);
        player_speed.x = 0.0;
        player_speed.y = 0.0;
      }
      else if (first == player)
      {
        first->move(mpv);
        player_speed.x = 0.0;
        player_speed.y = 0.0;
      }
      if (counter++ > 50 && run_anim == true)
        run_anim = false;
    }
    else {
      std::cout << "Something wrong happend with pointers\n";
    }
  }

  void on_static_static_collision(ge::DrawablePtr static1, ge::DrawablePtr static2)
  {

  }
  void on_static_dynamic_collision(ge::DrawablePtr static_obj, ge::DrawablePtr dynamic_obj)
  {

  }
  void on_dynamic_dynamic_collision(ge::DrawablePtr dynamic1, ge::DrawablePtr dynamic2)
  {

  }

  void on_keyboard_event(const std::shared_ptr<ge::events::keyboard_event>& event) override
  {
    if (event->key == GLFW_KEY_W) {
      player->move({ 0.f, 0.1f });
      player_speed.y = 8;
    }
    else if (event->key == GLFW_KEY_S) {
      player->move({ 0.f, -0.1f });
    }
    if (event->key == GLFW_KEY_A) {
      player->move({ -0.1f, 0.f });
      player_speed.x = -1;
    }
    else if (event->key == GLFW_KEY_D) {
      player->move({ 0.1f, 0.f });
      player_speed.x = 1;
    }
  }
  void on_mouse_click_event(const std::shared_ptr<ge::events::mouse_click_event>& event) override
  {
    if (event->action == ge::events::mouse_click_event::actions::press &&
      event->button == GLFW_MOUSE_BUTTON_LEFT) {

    }
  }
  void on_mouse_move_event(const std::shared_ptr<ge::events::mouse_move_event>& event) override
  {
    //player->set_pos({ event->x, event->y });
    //std::cout << "Hello, world!\n";
  }
  void on_scroll_move_event(const std::shared_ptr<ge::events::scroll_move_event>& event) override
  {

  }

  const ge::Camera& get_camera() const override
  {
    return camera;
  }

private:
  glm::vec2 player_speed = { 0.0, 0.0 };
  //double player_speed = 0.0;
  ge::DrawablePtr player;
  ge::DrawablePtr floor;
  bool run_anim = true;

};
