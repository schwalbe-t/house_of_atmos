
#include <engine/window.hpp>
#include <engine/rendering.hpp>

// for testing
#include <glad/gl.h>
#include <engine/logging.hpp>
#include <GLFW/glfw3.h>

using namespace houseofatmos;

int main(int argc, char** argv) {
    auto window = engine::Window(1280, 720, "House of Atmos", true);
    auto shader = engine::Shader(
        "#version 430 \n"
        "in vec3 v_pos; \n"
        "in vec3 v_color; \n"
        "out vec3 f_color; \n"
        "void main() { \n"
        "    f_color = v_color; \n"
        "    gl_Position = vec4(v_pos, 1); \n"
        "}",
        "#version 430 \n"
        "in vec3 f_color; \n"
        "out vec4 color; \n"
        "void main() { \n"
        "    color = vec4(f_color, 1); \n"
        "}"
    );
    auto mesh = engine::Mesh { 3, 3 };
    mesh.add_element(
        mesh.add_vertex({ -0.5,  0.5, 0,   1, 0, 0 }),
        mesh.add_vertex({  0.5,  0.5, 0,   0, 1, 0 }),
        mesh.add_vertex({  0.0, -0.5, 0,   0, 0, 1 })
    );
    mesh.submit();
    while(window.is_open()) {
        // for testing, needs proper methods later
        i32 width;
        i32 height;
        glfwGetFramebufferSize(
            (GLFWwindow*) window.internal_ptr(), &width, &height
        );
        glViewport(0, 0, width, height);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader.internal_prog_id());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.internal_elem_id());
        glBindVertexArray(mesh.internal_varr_id());
        glDrawElements(
            GL_TRIANGLES, mesh.internal_buffered_indices(), 
            GL_UNSIGNED_SHORT, 0
        );
        glBindVertexArray(GL_NONE);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);
        glUseProgram(GL_NONE);
        glfwSwapBuffers((GLFWwindow*) window.internal_ptr());
    }
}