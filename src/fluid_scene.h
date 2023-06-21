#pragma once

#include "core/scene.h"
#include "fluid_sims.h"


struct Scene : public ge::NewScene
{
  bool drawPressure = false;
  bool drawSmoke = true;
  bool drawStreamlines = false;

  std::size_t iterations = 100;

  float overRelaxation = 1.0f;

  float dt = 1.0f / 60;

  glm::vec2 gravity = { 0.0f, 0.0f };

  FluidSims::scene_type_t scene_type = FluidSims::scene_type_t::wind_tunnel;

  std::size_t frameCount = 0;

  std::shared_ptr<FluidSims::Fluid> fluid = nullptr;

  FluidSims::RigidBody obstacle{ FluidSims::RigidBody::none, { 0.0f, 0.0f}, { 0.0f, 0.0f }, 1.0f, { 10.0f, 10.0f} };

  glm::vec2 obstacle_new_pos{ 0.0f, 0.0f };
  FluidSims::RigidBody::type_t obstacle_new_type = FluidSims::RigidBody::none;

  bool shouldReset = false;
  bool lastShouldReset = false;

  std::vector<float> points;
  std::vector<float> lines;

  entt::entity drawable_points_entt = entt::null;
  entt::entity drawable_lines_entt = entt::null;
  entt::entity obstacle_entt = entt::null;

  ge::SmartPtr<ge::Window> m_window;

  glm::vec2 size_multiplier = { 1.0f, 1.0f };

  Scene(ge::SmartPtr<ge::Window> window)
    : m_window(std::move(window))
  {
  }
  float sign(const glm::vec2 p1, const glm::vec2 p2, const glm::vec2 p3)
  {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
  }

  bool PointInTriangle(const glm::vec2 pt, const glm::vec2 v1, const glm::vec2 v2, const glm::vec2 v3)
  {
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(pt, v1, v2);
    d2 = sign(pt, v2, v3);
    d3 = sign(pt, v3, v1);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
  }

  void setObstacleNone()
  {
    const std::size_t n = fluid->numY;

    for (std::size_t i = 1; i < fluid->numX - 2; i++) {
      for (std::size_t j = 1; j < fluid->numY - 2; j++) {

        fluid->solid[i * n + j] = 1.0f;
      }
    }
  }

  void setObstacleTriangle(float x, float y, bool reset)
  {
    float vx = 0.0f;
    float vy = 0.0f;

    if (!reset) {
      vx = (x - obstacle.pos.x) / dt * 2.0f;
      vy = (y - obstacle.pos.y) / dt * 2.0f;
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

        //TODO: fix naming
        glm::vec2 dxy = { dx, dy };
        glm::vec2 pos = { x * fluid->numX, y * fluid->numY };
        glm::vec2 posIJ = { i, j };

        glm::vec2 point1 = pos;
        point1.x -= obstacle.size.x;

        glm::vec2 point2 = pos;
        point2.x += obstacle.size.x;
        point2.y -= obstacle.size.y;

        glm::vec2 point3 = pos;
        point3.x += obstacle.size.x;
        point3.y += obstacle.size.y;

        float dist = dx * dx + dy * dy;

        if (PointInTriangle(posIJ, point1, point2, point3)) {
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

    if (registry->valid(obstacle_entt))
    {
      ge::SpriteComponent& sprite = registry->get<ge::SpriteComponent>(obstacle_entt);
      sprite.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\triangle.png");
    }
  }

  void setObstacleCircle(float x, float y, bool reset)
  {
    float vx = 0.0f;
    float vy = 0.0f;

    if (!reset) {
      vx = (x - obstacle.pos.x) / dt * 2.0f;
      vy = (y - obstacle.pos.y) / dt * 2.0f;
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

        float dist = dx * dx + dy * dy;

        if (dist < r * r) {
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

    if (registry->valid(obstacle_entt))
    {
      ge::SpriteComponent& sprite = registry->get<ge::SpriteComponent>(obstacle_entt);
      sprite.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\circle.png");
    }
  }

  void setObstacleSquare(float x, float y, bool reset)
  {
    float vx = 0.0f;
    float vy = 0.0f;

    if (!reset) {
      vx = (x - obstacle.pos.x) / dt * 2.0f;
      vy = (y - obstacle.pos.y) / dt * 2.0f;
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

    if (registry->valid(obstacle_entt))
    {
      ge::SpriteComponent& sprite = registry->get<ge::SpriteComponent>(obstacle_entt);
      sprite.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\square.png");
    }
  }

  void setObstacle(float x, float y, bool reset) {

    if (obstacle.type == FluidSims::RigidBody::circle)
    {
      setObstacleCircle(x, y, reset);
    }
    else if (obstacle.type == FluidSims::RigidBody::square)
    {
      setObstacleSquare(x, y, reset);
    }
    else if (obstacle.type == FluidSims::RigidBody::triangle)
    {
      setObstacleTriangle(x, y, reset);
    }
    else if (obstacle.type == FluidSims::RigidBody::none)
    {
      setObstacleNone();
    }
    else
    {
      assert(false && "Shape not implemented yet");
    }

    if (registry->valid(obstacle_entt))
    {
      ge::TransformComponent& transform = registry->get<ge::TransformComponent>(obstacle_entt);

      glm::vec2 pos;

      pos.x = (x * fluid->numX);
      pos.y = (y * fluid->numY);

      pos.x -= fluid->numX / 2.0f + 1;
      pos.y -= fluid->numY / 2.0f + 10;

      transform.translation.x = pos.x * size_multiplier.x;
      transform.translation.y = pos.y * size_multiplier.x;


      if (obstacle.type == FluidSims::RigidBody::circle)
      {
        transform.scale = { m_window->width() * obstacle.radius / 100.0f, m_window->width() * obstacle.radius / 100.0f, 1.0f };
      }
      else if (obstacle.type == FluidSims::RigidBody::square)
      {
        transform.scale = { m_window->width() * obstacle.size.x / 100.0f, m_window->width() * obstacle.size.y / 100.0f, 1.0f };
      }
      else if (obstacle.type == FluidSims::RigidBody::triangle)
      {
        transform.scale = { m_window->width() * obstacle.size.x / 100.0f, m_window->width() * obstacle.size.y / 100.0f, 1.0f };
      }
      else if (obstacle.type == FluidSims::RigidBody::none)
      {
        transform.scale = { 0.0f, 0.0f, 0.0f };
      }
    }
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
    obstacle.size = { 10.0f, 10.0f };
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
    obstacle.size = { 10.0f, 10.0f };
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
  }

  void processInput(GLFWwindow* window)
  {
    obstacle_new_pos = obstacle.pos;

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

    const int lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (lmb == GLFW_PRESS)
    {
      double xpos, ypos;

      glfwGetCursorPos(window, &xpos, &ypos);

      if (ypos < this->m_window->height() * 0.2f || 
        xpos < this->m_window->width() * 0.1f)
        return;

      xpos = xpos / m_window->width();
      ypos = ypos / m_window->height();

      obstacle_new_pos.x = xpos;
      obstacle_new_pos.y = (1.0 - ypos) * 1.2f;

      lastShouldReset = shouldReset;
      shouldReset = false;

      return;
    }
    else if (lmb == GLFW_RELEASE)
    {
      lastShouldReset = shouldReset;
      shouldReset = true;
    }

  }

  void on_create() override
  {
    float field_width = 220;
    float field_height = 100;

    fluid = std::make_shared<FluidSims::Fluid>(new FluidSims::IntegratorEuler(), 1000.0f, field_width, field_height, 1.0f / field_height);

    setup_scene(FluidSims::scene_type_t::wind_tunnel, FluidSims::RigidBody::circle);

    camera.pos.z = 10.0f;
    camera.center = glm::vec3{ 0.0f, 0.0f, -1.0f };
    camera.up = glm::vec3{ 0.0f, 1.0f, 0.0f };

    camera.pos.x = 0.0f;
    camera.pos.y = 0.0f;

    drawable_points_entt = registry->create();
    registry->emplace<ge::NewDrawable>(drawable_points_entt);

    drawable_lines_entt = registry->create();
    registry->emplace<ge::NewDrawable>(drawable_lines_entt);

    obstacle_entt = registry->create();
    ge::SpriteComponent& sprite = registry->emplace<ge::SpriteComponent>(obstacle_entt);

    sprite.texture.load("d:\\dev\\VS\\VSProjects\\GameEngine\\GameEngine\\GameEngine\\src\\render\\opengl\\shaders\\images\\circle.png");

    ge::TransformComponent transform;
    transform.translation = { 0.0f, 0.0f, 0.0f };
    transform.rotation_angle = 0.0f;
    transform.rotation_axes = { 0.0f, 0.0f, 1.0f };
    transform.scale = { m_window->width() * 0.15f, m_window->width() * 0.15f, 1.0f };

    registry->emplace<ge::TransformComponent>(obstacle_entt, transform);
  }

  void on_update(const double dt) override
  {
    size_multiplier.x = m_window->width() / static_cast<float>(fluid->numX);
    size_multiplier.y = m_window->height() / static_cast<float>(fluid->numY);

    processInput(m_window->native());

    points.clear();
    lines.clear();

    draw_field_to_vector(points, size_multiplier);

    if (this->drawStreamlines)
    {
      draw_streamlines_to_vector(*fluid, lines, size_multiplier.x, 5, 0.02f);
    }

    if (this->obstacle.pos != this->obstacle_new_pos || this->obstacle.type != this->obstacle_new_type || this->shouldReset != this->lastShouldReset)
    {
      this->obstacle.type = this->obstacle_new_type;
      this->setObstacle(this->obstacle_new_pos.x, this->obstacle_new_pos.y, false);
      this->obstacle.pos = this->obstacle_new_pos;
    }


    fluid->simulate(this->dt, this->gravity.y, this->iterations);

    this->frameCount++;


    ge::NewDrawable& drawable_points = registry->get<ge::NewDrawable>(drawable_points_entt);
    ge::NewDrawable& drawable_lines = registry->get<ge::NewDrawable>(drawable_lines_entt);

    drawable_points.vertices = points;
    drawable_points.m_count = drawable_points.vertices.size() / 6;
    drawable_points.mode = GL_TRIANGLES;

    drawable_lines.vertices = lines;
    drawable_lines.m_count = drawable_lines.vertices.size() / 6;
    drawable_lines.mode = GL_LINES;


    ImGui::SetWindowFontScale(1.5f);

    ImGui::BeginGroup();
    ImGui::BeginGroup();
    ImGui::Button("Wind tunnel", { 150.0f, 50.0f });
    if (ImGui::IsItemActive())
    {
      this->setup_scene(FluidSims::scene_type_t::wind_tunnel, this->obstacle.type);
    }

    ImGui::SameLine();

    ImGui::Button("Paint", { 150.0f, 50.0f });
    if (ImGui::IsItemActive())
    {
      this->setup_scene(FluidSims::scene_type_t::paint, this->obstacle.type);
    }
    ImGui::EndGroup();

    ImGui::Spacing();

    const char* items[] = { "Circle", "Square", "Triangle", "None" };
    static const char* current_item = items[0];

    if (ImGui::BeginCombo("Rigid body", current_item, ImGuiComboFlags_NoPreview))
    {
      for (int n = 0; n < IM_ARRAYSIZE(items); n++)
      {
        bool is_selected = (current_item == items[n]);
        if (ImGui::Selectable(items[n], is_selected))
          current_item = items[n];
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();

      if (current_item == items[0])
        this->obstacle_new_type = FluidSims::RigidBody::circle;
      else if (current_item == items[1])
        this->obstacle_new_type = FluidSims::RigidBody::square;
      else if (current_item == items[2])
        this->obstacle_new_type = FluidSims::RigidBody::triangle;
      else if (current_item == items[3])
        this->obstacle_new_type = FluidSims::RigidBody::none;

    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Checkbox("Draw pressure", &this->drawPressure);
    ImGui::Checkbox("Draw smoke", &this->drawSmoke);
    ImGui::Checkbox("Draw streamlines", &this->drawStreamlines);
    ImGui::EndGroup();

  }

  void draw_field_to_vector(std::vector<float>& points, const glm::vec2 size_multiplier)
  {
    const std::vector<float> pressure = fluid->pressure;
    float minP = pressure[0];
    float maxP = pressure[0];

    for (float pressure : pressure)
    {
      minP = std::min(minP, pressure);
      maxP = std::max(maxP, pressure);
    }

    auto add_point_tri = [&points](const glm::vec3& location, const glm::vec3& color)
    {
      points.push_back(location.x);
      points.push_back(location.y);
      points.push_back(location.z);

      points.push_back(color[0]);
      points.push_back(color[1]);
      points.push_back(color[2]);
    };

    for (std::size_t x = 0; x < fluid->numX / 1; ++x)
    {
      for (std::size_t y = 0; y < fluid->numY / 1; ++y)
      {
        glm::vec3 color;
        float smoke = fluid->smoke[x * fluid->numY + y];
        float pressure = fluid->pressure[x * fluid->numY + y];
        float solid = fluid->solid[x * fluid->numY + y];

        if (this->drawPressure)
        {
          color = getSciColor(pressure, minP, maxP);
          if (this->drawSmoke)
          {
            color.r -= smoke;
            color.g -= smoke;
            color.b -= smoke;
          }

        }
        else if (this->drawSmoke) {
          color.r = smoke;
          color.g = smoke;
          color.b = smoke;

          if (this->scene_type == FluidSims::scene_type_t::paint)
            color = getSciColor(smoke, 0.0, 1.0);
        }
        else {
          color.r = 0.0f;
          color.g = 0.0f;
          color.b = 0.0f;
        }


        //if (solid < 0.9f)
        //{
        //  if (this->scene_type == FluidSims::scene_type_t::paint)
        //  {
        //    color.r = 0.6f;
        //    color.g = 0.6f;
        //    color.b = 0.6f;
        //  }
        //  else {
        //    color.r = 0.6f;
        //    color.g = 0.6f;
        //    color.b = 0.6f;
        //  }
        //}


        glm::vec3 location1;
        glm::vec3 location2;
        glm::vec3 location3;
        glm::vec3 location4;
        location1 = {
          x * 1.0f + 0.0f,
          y * 1.0f + 0.0f,
          0.0f * 1.0f + 00.0f
        };

        location2 = {
          x * 1.0f + 1.0f,
          y * 1.0f + 0.0f,
          0.0f * 1.0f + 00.0f
        };

        location3 = {
          x * 1.0f + 0.0f,
          y * 1.0f + 1.0f,
          0.0f * 1.0f + 00.0f
        };

        location4 = {
          x * 1.0f + 1.0f,
          y * 1.0f + 1.0f,
          0.0f * 1.0f + 00.0f
        };

        location1.x -= fluid->numX / 2.0f + 1;
        location1.y -= fluid->numY / 2.0f + 10;
        location2.x -= fluid->numX / 2.0f + 1;
        location2.y -= fluid->numY / 2.0f + 10;
        location3.x -= fluid->numX / 2.0f + 1;
        location3.y -= fluid->numY / 2.0f + 10;
        location4.x -= fluid->numX / 2.0f + 1;
        location4.y -= fluid->numY / 2.0f + 10;


        location1.x *= size_multiplier.x;
        location1.y *= size_multiplier.x;
        location2.x *= size_multiplier.x;
        location2.y *= size_multiplier.x;
        location3.x *= size_multiplier.x;
        location3.y *= size_multiplier.x;
        location4.x *= size_multiplier.x;
        location4.y *= size_multiplier.x;

        add_point_tri(location1, color);
        add_point_tri(location2, color);
        add_point_tri(location3, color);

        add_point_tri(location2, color);
        add_point_tri(location3, color);
        add_point_tri(location4, color);
      }
    }
  }

  void draw_streamlines_to_vector(const FluidSims::Fluid& fluid, std::vector<float>& lines, const float size_multiplier, const std::size_t num_segs, const float seg_length)
  {
    auto add_point_lines = [&lines, size_multiplier](const glm::vec3& location, const glm::vec3& color)
    {
      lines.push_back(location.x * size_multiplier);
      lines.push_back(location.y * size_multiplier);
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

};