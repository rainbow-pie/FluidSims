#include "gl/glew.h"
#include "glfw/glfw3.h"

#include "core/Application.h"

#include "fluid_scene.h"

#include <chrono>
#include <iostream>
#include <functional>

#include <vector>
#include <iterator>


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
  int width = 1920;
  int height = 1080;
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

  std::shared_ptr<Scene> scene1 = std::make_shared<Scene>(main_window);
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
