#ifndef SIMPLE_MODULE_H
#	define SIMPLE_MODULE_H

#	include <flecs.h>

// clang-format off
#include "glad/glad.h"
#include "GLFW/glfw3.h"
// clang-format on

namespace simple
{

struct Position {
	double x, y;
};

struct Velocity {
	double x, y;
};

struct InputState { };
struct WindowInstance { };

struct GraphicsModule {
	GraphicsModule(flecs::world& ecs)
	{
		ecs.module<GraphicsModule>();

		ecs.component<InputState>();
		ecs.component<WindowInstance>();

		ecs.system("GLFW startup").kind(flecs::OnStart).iter([&](flecs::iter& it) {
			glfwInit(); //TODO: check errors
			ecs.set<InputState>({});

			auto w = glfwCreateWindow(640, 480, "", 0, 0); //TODO: check errors
			glfwSetWindowUserPointer(w, ecs.c_ptr());

			glfwSetKeyCallback(
				w, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
					auto ecs = flecs::world(
						reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
					auto input_state = ecs.get_mut<InputState>();
					input_state->keyboard[key] = (action != GLFW_RELEASE);
				});

			glfwSetMouseButtonCallback(w, [](GLFWwindow* window, int button, int action, int mods) {
				auto ecs = flecs::world(
					reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
				auto input_state = ecs.get_mut<InputState>();
				input_state->mouse[button] = (action != GLFW_RELEASE);
			});

			glfwSetScrollCallback(w, [](GLFWwindow* window, double xoffset, double yoffset) {
				auto ecs = flecs::world(
					reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
				auto input_state = ecs.get_mut<InputState>();
				input_state->mouse_scroll = {xoffset, yoffset};
			});

			glfwSetCursorPosCallback(w, [](GLFWwindow* window, double xpos, double ypos) {
				auto ecs = flecs::world(
					reinterpret_cast<flecs::world_t*>(glfwGetWindowUserPointer(window)));
				auto input_state = ecs.get_mut<InputState>();
				input_state->mouse_pos = {xpos, ypos};
			});

			glfwMakeContextCurrent(w);
			glewExperimental = true;
			glewInit(); //TODO: check errors

			ecs.set<WindowInstance>({w});
		});

		ecs.system<InputState>("Scroll nullifier")
			.kind(flecs::PreUpdate)
			.each([](flecs::entity, InputState& w) { w.mouse_scroll = glm::dvec2(0.0, 0.0); });

		ecs.system<const WindowInstance>("Update windows")
			.kind(flecs::OnUpdate)
			.each([](flecs::entity, const WindowInstance& w) {
				int width, height;
				glfwGetFramebufferSize(w.id, &width, &height);

				glViewport(0, 0, width, height);
				glClear(GL_COLOR_BUFFER_BIT);

				glfwSwapBuffers(w.id);
				glfwPollEvents();
			});
	}
};

} // namespace simple