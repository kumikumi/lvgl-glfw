#define GL_SILENCE_DEPRECATION
#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "lvgl.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static GLuint texture;
static lv_draw_buf_t draw_buf;
static lv_color32_t *buf;
static lv_display_t *disp;
static lv_obj_t *resolution_label;

static void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    // Update the texture with the new frame buffer content
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, area->x1, area->y1, 
                    lv_area_get_width(area), lv_area_get_height(area), 
                    GL_BGRA, GL_UNSIGNED_BYTE, px_map);

    lv_display_flush_ready(disp);
}

static void my_mouse_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    GLFWwindow* window = (GLFWwindow*)lv_indev_get_user_data(indev);
    
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    data->point.x = (int16_t)x;
    data->point.y = (int16_t)y;
    data->state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? 
                  LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void update_resolution_text(int width, int height)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Resolution: %dx%d", width, height);
    lv_label_set_text(resolution_label, buf);
}

static void window_resize_callback(GLFWwindow* window, int width, int height)
{
    // Update OpenGL viewport
    glViewport(0, 0, width, height);

    // Update LVGL display resolution
    lv_display_set_resolution(disp, width, height);

    // Resize the draw buffer
    lv_draw_buf_destroy(&draw_buf);
    buf = realloc(buf, width * height * sizeof(lv_color32_t));
    lv_draw_buf_init(&draw_buf, width, height, LV_COLOR_FORMAT_NATIVE, 
                     width * sizeof(lv_color32_t),
                     buf, width * height * sizeof(lv_color32_t));
    lv_display_set_buffers(disp, buf, NULL, width * height * sizeof(lv_color32_t), LV_DISPLAY_RENDER_MODE_DIRECT);

    // Resize the OpenGL texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Update the resolution text
    update_resolution_text(width, height);
}

int main(void)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LVGL with GLFW", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, window_resize_callback);

    // Initialize LVGL
    lv_init();

    // Initialize the display buffer
    buf = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(lv_color32_t));
    lv_draw_buf_init(&draw_buf, WINDOW_WIDTH, WINDOW_HEIGHT, LV_COLOR_FORMAT_NATIVE, 
                     WINDOW_WIDTH * sizeof(lv_color32_t),
                     buf, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(lv_color32_t));

    // Initialize the display driver
    disp = lv_display_create(WINDOW_WIDTH, WINDOW_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf, NULL, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(lv_color32_t), LV_DISPLAY_RENDER_MODE_DIRECT);

    // Set the resolution of the display
    lv_display_set_resolution(disp, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize the input device driver
    lv_indev_t * mouse_indev = lv_indev_create();
    lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse_indev, my_mouse_read);
    lv_indev_set_user_data(mouse_indev, window);

    // Create a label for the resolution
    resolution_label = lv_label_create(lv_scr_act());
    lv_obj_align(resolution_label, LV_ALIGN_TOP_LEFT, 10, 10);
    update_resolution_text(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Create a label
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello, World!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);

    // Create three buttons with different colors
    lv_obj_t * btn_red = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_red, LV_ALIGN_CENTER, -100, 40);
    lv_obj_set_style_bg_color(btn_red, lv_color_hex(0xFF0000), 0);

    lv_obj_t * btn_green = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_green, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn_green, lv_color_hex(0x00FF00), 0);

    lv_obj_t * btn_blue = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_blue, LV_ALIGN_CENTER, 100, 40);
    lv_obj_set_style_bg_color(btn_blue, lv_color_hex(0x0000FF), 0);

    // Create an OpenGL texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    printf("GLFW Window: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("LVGL Display: %dx%d\n", lv_display_get_horizontal_resolution(disp), lv_display_get_vertical_resolution(disp));
    printf("OpenGL Texture: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("LVGL Color Depth: %d bits\n", LV_COLOR_DEPTH);

    while (!glfwWindowShouldClose(window)) {
        lv_timer_handler();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw a fullscreen quad with the LVGL texture
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(-1, -1);
        glTexCoord2f(1, 1); glVertex2f(1, -1);
        glTexCoord2f(1, 0); glVertex2f(1, 1);
        glTexCoord2f(0, 0); glVertex2f(-1, 1);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();

        lv_tick_inc(16); // Assuming 60 FPS
    }

    // Clean up
    free(buf);
    glfwTerminate();
    return 0;
}
