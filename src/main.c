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
static lv_obj_t *frame_counter_label;
static lv_obj_t *selectable_label;
static uint32_t frame_count = 0;

static int selection_start = LV_LABEL_TEXT_SELECTION_OFF;
static int selection_end = LV_LABEL_TEXT_SELECTION_OFF;

static void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    int32_t width = lv_display_get_horizontal_resolution(disp);
    int32_t height = lv_display_get_vertical_resolution(disp);

    // Calculate the start position of the updated area in px_map
    int32_t stride = width * sizeof(lv_color32_t);  // Bytes per row
    uint8_t *start_pos = px_map + (area->y1 * stride) + (area->x1 * sizeof(lv_color32_t));

    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);  // Set the row length to the full width of the texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Ensure 1-byte alignment

    glTexSubImage2D(GL_TEXTURE_2D, 0, area->x1, area->y1,
                    lv_area_get_width(area), lv_area_get_height(area),
                    GL_RGBA, GL_UNSIGNED_BYTE, start_pos);

    // Reset the row length
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

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

static void update_frame_counter()
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Frames: %u", frame_count++);
    lv_label_set_text(frame_counter_label, buf);
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

static void label_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_point_t p;

    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_active(), &p);
        selection_start = lv_label_get_letter_on(obj, &p, false);
        selection_end = LV_LABEL_TEXT_SELECTION_OFF;
        lv_label_set_text_selection_start(obj, selection_start);
        lv_label_set_text_selection_end(obj, selection_end);
    }
    else if(code == LV_EVENT_PRESSING) {
        lv_indev_get_point(lv_indev_active(), &p);
        selection_end = lv_label_get_letter_on(obj, &p, false);
        lv_label_set_text_selection_end(obj, selection_end);
    }
    else if(code == LV_EVENT_RELEASED) {
        if(selection_start == selection_end) {
            selection_start = LV_LABEL_TEXT_SELECTION_OFF;
            selection_end = LV_LABEL_TEXT_SELECTION_OFF;
            lv_label_set_text_selection_start(obj, LV_LABEL_TEXT_SELECTION_OFF);
            lv_label_set_text_selection_end(obj, LV_LABEL_TEXT_SELECTION_OFF);
        }
    }
}

static void create_gradient_background(lv_obj_t * parent)
{
    static const lv_color_t grad_colors[2] = {
        LV_COLOR_MAKE(0x9B, 0x18, 0x42),
        LV_COLOR_MAKE(0x00, 0x00, 0x00),
    };

    int32_t width = lv_display_get_horizontal_resolution(NULL);
    int32_t height = lv_display_get_vertical_resolution(NULL);

    static lv_style_t style;
    lv_style_init(&style);

    static lv_grad_dsc_t grad;
    lv_gradient_init_stops(&grad, grad_colors, NULL, NULL, sizeof(grad_colors) / sizeof(lv_color_t));
    lv_grad_radial_init(&grad, LV_GRAD_CENTER, LV_GRAD_CENTER, LV_GRAD_RIGHT, LV_GRAD_BOTTOM, LV_GRAD_EXTEND_PAD);
    lv_style_set_bg_grad(&style, &grad);

    lv_obj_t * obj = lv_obj_create(parent);
    lv_obj_add_style(obj, &style, 0);
    lv_obj_set_size(obj, width, height);
    lv_obj_center(obj);
    lv_obj_move_to_index(obj, 0);  // Move to the background
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

    // Create gradient background
    create_gradient_background(lv_scr_act());

    // Create a label for the resolution
    resolution_label = lv_label_create(lv_scr_act());
    lv_obj_align(resolution_label, LV_ALIGN_TOP_LEFT, 10, 10);
    update_resolution_text(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Create a label for the frame counter
    frame_counter_label = lv_label_create(lv_scr_act());
    lv_obj_align(frame_counter_label, LV_ALIGN_TOP_LEFT, 10, 40);
    update_frame_counter();

    // Create a selectable label
    selectable_label = lv_label_create(lv_scr_act());
    lv_label_set_text(selectable_label, "Hello, World! This text is selectable.");
    lv_obj_align(selectable_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_flag(selectable_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(selectable_label, label_event_cb, LV_EVENT_ALL, NULL);

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

        // Update the frame counter
        update_frame_counter();

        glfwSwapBuffers(window);
        glfwPollEvents();

        lv_tick_inc(16); // Assuming 60 FPS
    }

    // Clean up
    free(buf);
    glfwTerminate();
    return 0;
}