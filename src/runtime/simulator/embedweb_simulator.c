#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ew_types.h"
#include "ew_tree.h"
#include "ew_layout.h"
#include "ew_raster.h"
#include "ew_hal.h"
#include "ew_input.h"
#include "ew_resource.h"

#ifdef HAS_SDL2
#include <SDL2/SDL.h>
#endif

#define SIM_DEFAULT_WIDTH  480
#define SIM_DEFAULT_HEIGHT 320
#define SIM_SLICE_HEIGHT   16
#define SIM_EVENT_QUEUE_CAP 32

/* Circular input event queue */
typedef struct {
    ew_input_event_t events[SIM_EVENT_QUEUE_CAP];
    int head;
    int tail;
} sim_event_queue_t;

static void queue_init(sim_event_queue_t* q) {
    q->head = 0;
    q->tail = 0;
}

static bool queue_push(sim_event_queue_t* q, ew_input_event_t evt) {
    int next = (q->tail + 1) % SIM_EVENT_QUEUE_CAP;
    if (next == q->head) return false; /* Full */
    q->events[q->tail] = evt;
    q->tail = next;
    return true;
}

static bool queue_pop(sim_event_queue_t* q, ew_input_event_t* evt) {
    if (q->head == q->tail) return false; /* Empty */
    *evt = q->events[q->head];
    q->head = (q->head + 1) % SIM_EVENT_QUEUE_CAP;
    return true;
}

/* Simulator context */
typedef struct {
    int width;
    int height;
    int slice_height;
    ew_tree_t tree;
    ew_engine_t engine;
    uint16_t* slice_buffer;
    uint16_t* framebuffer; /* Host display buffer for texture upload */
    sim_event_queue_t input_queue;
    int16_t btn_inc_idx;
    int16_t btn_reset_idx;
    int16_t pbar_idx;
    int16_t label_val_idx;
    int current_val;
    int flush_count;
    bool headless;
    bool test_mode;
    bool interaction_passed;
} sim_context_t;

static sim_context_t g_sim;

/* HAL Display Flush Callback */
static void sim_display_flush(ew_display_driver_t* disp, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    (void)disp;
    g_sim.flush_count++;
    if (!g_sim.framebuffer) return;

    for (int line = 0; line < h; line++) {
        int dst_y = y + line;
        if (dst_y < 0 || dst_y >= g_sim.height) continue;
        int dst_offset = dst_y * g_sim.width + x;
        int src_offset = line * w;
        memcpy(&g_sim.framebuffer[dst_offset], &pixels[src_offset], w * sizeof(uint16_t));
    }
}

/* HAL Input Poll Callback */
static bool sim_input_poll(ew_input_driver_t* drv, ew_input_event_t* event) {
    (void)drv;
    return queue_pop(&g_sim.input_queue, event);
}

/* Test Image Asset: 16x16 icon */
static const uint16_t s_icon_pixels[256] = {
    0x0001, 0x0001, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0001, 0x0001,
    0x0001, 0xFFFF, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0xFFFF, 0x0001,
    0xFFFF, 0x001F, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x07E0, 0xF800, 0xF800, 0x07E0, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x07E0, 0xF800, 0xF800, 0x07E0, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x07E0, 0x001F, 0xFFFF,
    0xFFFF, 0x001F, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x001F, 0xFFFF,
    0x0001, 0xFFFF, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0xFFFF, 0x0001,
    0x0001, 0x0001, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0001, 0x0001
};

static const ew_image_asset_t s_icon_asset = {
    .width = 16,
    .height = 16,
    .format = EW_IMAGE_FORMAT_RGB565_CHROMA,
    .chroma_key = 0x0001,
    .pixels = s_icon_pixels
};

/* Button Click Event Callbacks */
static void on_btn_increment_click(ew_node_t* node, uint8_t event_type, void* user_data) {
    (void)node; (void)user_data;
    if (event_type != EW_EVENT_CLICK) return;

    g_sim.current_val = (g_sim.current_val + 10);
    if (g_sim.current_val > 100) g_sim.current_val = 0;

    ew_node_set_progress(&g_sim.tree, g_sim.pbar_idx, (int16_t)g_sim.current_val);

    char val_str[32];
    snprintf(val_str, sizeof(val_str), "Value: %d%%", g_sim.current_val);
    ew_node_set_text(&g_sim.tree, g_sim.label_val_idx, val_str);

    /* Invalidate dirty regions */
    ew_engine_invalidate_node(&g_sim.engine, g_sim.pbar_idx);
    ew_engine_invalidate_node(&g_sim.engine, g_sim.label_val_idx);

    printf("[SIMULATOR] Button Click: Incremented progress to %d%%\n", g_sim.current_val);
    g_sim.interaction_passed = true;
}

static void on_btn_reset_click(ew_node_t* node, uint8_t event_type, void* user_data) {
    (void)node; (void)user_data;
    if (event_type != EW_EVENT_CLICK) return;

    g_sim.current_val = 0;
    ew_node_set_progress(&g_sim.tree, g_sim.pbar_idx, 0);
    ew_node_set_text(&g_sim.tree, g_sim.label_val_idx, "Value: 0%");

    ew_engine_invalidate_node(&g_sim.engine, g_sim.pbar_idx);
    ew_engine_invalidate_node(&g_sim.engine, g_sim.label_val_idx);

    printf("[SIMULATOR] Button Click: Reset progress to 0%%\n");
}

/* Build demonstration UI */
static void sim_build_ui(void) {
    ew_tree_init(&g_sim.tree);

    /* 0. Root Screen */
    int16_t root = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&g_sim.tree, root);
    n_root->rect = (ew_rect_t){ 0, 0, (int16_t)g_sim.width, (int16_t)g_sim.height };
    n_root->style.has_bg = 1;
    n_root->style.bg_color = ew_color_rgb565(18, 24, 38);

    /* 1. Header Card Container */
    int16_t card = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_card = ew_tree_get_node(&g_sim.tree, card);
    n_card->rect = (ew_rect_t){ 20, 20, (int16_t)(g_sim.width - 40), (int16_t)(g_sim.height - 40) };
    n_card->style.has_bg = 1;
    n_card->style.bg_color = ew_color_rgb565(30, 41, 59);
    n_card->style.has_border = 1;
    n_card->style.border_color = ew_color_rgb565(59, 130, 246);
    n_card->style.border_width = 1;
    n_card->style.border_radius = 8;
    ew_tree_add_child(&g_sim.tree, root, card);

    /* 2. Title Label */
    int16_t title = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_LABEL);
    ew_node_t* n_title = ew_tree_get_node(&g_sim.tree, title);
    n_title->rect = (ew_rect_t){ 40, 35, 380, 20 };
    ew_node_set_text(&g_sim.tree, title, "EmbedWeb Desktop Simulator");
    n_title->style.text_color = ew_color_rgb565(255, 255, 255);
    ew_tree_add_child(&g_sim.tree, card, title);

    /* 3. Subtitle / Diagnostic info */
    int16_t sub = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_LABEL);
    ew_node_t* n_sub = ew_tree_get_node(&g_sim.tree, sub);
    n_sub->rect = (ew_rect_t){ 40, 58, 380, 16 };
    ew_node_set_text(&g_sim.tree, sub, "Native C 2D Engine with Dirty Redraw");
    n_sub->style.text_color = ew_color_rgb565(148, 163, 184);
    ew_tree_add_child(&g_sim.tree, card, sub);

    /* 4. Button Increment (+10) */
    g_sim.btn_inc_idx = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_BUTTON);
    ew_node_t* n_inc = ew_tree_get_node(&g_sim.tree, g_sim.btn_inc_idx);
    n_inc->rect = (ew_rect_t){ 40, 90, 140, 36 };
    ew_node_set_text(&g_sim.tree, g_sim.btn_inc_idx, "Step (+10)");
    n_inc->style.has_bg = 1;
    n_inc->style.bg_color = ew_color_rgb565(37, 99, 235);
    n_inc->style.text_color = ew_color_rgb565(255, 255, 255);
    n_inc->style.border_radius = 4;
    n_inc->touchable = 1;
    ew_node_set_event_cb(&g_sim.tree, g_sim.btn_inc_idx, on_btn_increment_click, NULL);
    ew_tree_add_child(&g_sim.tree, card, g_sim.btn_inc_idx);

    /* 5. Button Reset */
    g_sim.btn_reset_idx = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_BUTTON);
    ew_node_t* n_rst = ew_tree_get_node(&g_sim.tree, g_sim.btn_reset_idx);
    n_rst->rect = (ew_rect_t){ 190, 90, 100, 36 };
    ew_node_set_text(&g_sim.tree, g_sim.btn_reset_idx, "Reset");
    n_rst->style.has_bg = 1;
    n_rst->style.bg_color = ew_color_rgb565(220, 38, 38);
    n_rst->style.text_color = ew_color_rgb565(255, 255, 255);
    n_rst->style.border_radius = 4;
    n_rst->touchable = 1;
    ew_node_set_event_cb(&g_sim.tree, g_sim.btn_reset_idx, on_btn_reset_click, NULL);
    ew_tree_add_child(&g_sim.tree, card, g_sim.btn_reset_idx);

    /* 6. Image Widget */
    int16_t img = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_IMAGE);
    ew_node_t* n_img = ew_tree_get_node(&g_sim.tree, img);
    n_img->rect = (ew_rect_t){ 310, 100, 16, 16 };
    n_img->image_asset = &s_icon_asset;
    ew_tree_add_child(&g_sim.tree, card, img);

    /* 7. Progress Bar */
    g_sim.current_val = 30;
    g_sim.pbar_idx = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_PROGRESS_BAR);
    ew_node_t* n_pb = ew_tree_get_node(&g_sim.tree, g_sim.pbar_idx);
    n_pb->rect = (ew_rect_t){ 40, 145, 360, 20 };
    n_pb->style.has_bg = 1;
    n_pb->style.bg_color = ew_color_rgb565(15, 23, 42);
    n_pb->style.border_color = ew_color_rgb565(16, 185, 129);
    n_pb->style.border_width = 1;
    ew_node_set_progress(&g_sim.tree, g_sim.pbar_idx, (int16_t)g_sim.current_val);
    ew_tree_add_child(&g_sim.tree, card, g_sim.pbar_idx);

    /* 8. Value Readout Label */
    g_sim.label_val_idx = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_LABEL);
    ew_node_t* n_val = ew_tree_get_node(&g_sim.tree, g_sim.label_val_idx);
    n_val->rect = (ew_rect_t){ 40, 175, 200, 18 };
    ew_node_set_text(&g_sim.tree, g_sim.label_val_idx, "Value: 30%");
    n_val->style.text_color = ew_color_rgb565(52, 211, 153);
    ew_tree_add_child(&g_sim.tree, card, g_sim.label_val_idx);

    /* 9. Unsupported Component Demonstration (Rule 3 diagnostic requirement) */
    int16_t unsupp = ew_tree_alloc_node(&g_sim.tree, EW_WIDGET_UNSUPPORTED);
    if (unsupp >= 0) {
        ew_node_t* n_un = ew_tree_get_node(&g_sim.tree, unsupp);
        n_un->rect = (ew_rect_t){ 40, 215, 380, 16 };
        ew_node_set_text(&g_sim.tree, unsupp, "[Diagnostic] Unsupported component safely bypassed");
        n_un->style.text_color = ew_color_rgb565(248, 113, 113);
        ew_tree_add_child(&g_sim.tree, card, unsupp);
        printf("[EmbedWeb Simulator] Diagnostic: Node %d has EW_WIDGET_UNSUPPORTED; runtime safely bypassed rasterization.\n", unsupp);
    }
}

int main(int argc, char** argv) {
    g_sim.width = SIM_DEFAULT_WIDTH;
    g_sim.height = SIM_DEFAULT_HEIGHT;
    g_sim.slice_height = SIM_SLICE_HEIGHT;
    g_sim.headless = false;
    g_sim.test_mode = false;
    g_sim.interaction_passed = false;
    g_sim.flush_count = 0;
    queue_init(&g_sim.input_queue);

    int test_frames = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            g_sim.headless = true;
        } else if (strcmp(argv[i], "--test-interaction") == 0) {
            g_sim.test_mode = true;
            g_sim.headless = true;
        } else if (strcmp(argv[i], "--test-frames") == 0 && i + 1 < argc) {
            test_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            g_sim.width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            g_sim.height = atoi(argv[++i]);
        }
    }

    printf("[EmbedWeb Simulator] Target display: %dx%d, slice_lines=%d\n",
           g_sim.width, g_sim.height, g_sim.slice_height);

    /* Allocate slice buffer and host framebuffer */
    g_sim.slice_buffer = (uint16_t*)calloc(g_sim.width * g_sim.slice_height, sizeof(uint16_t));
    g_sim.framebuffer = (uint16_t*)calloc(g_sim.width * g_sim.height, sizeof(uint16_t));

    /* Initialize display & input drivers */
    ew_display_driver_t disp_drv = {
        .width = (int16_t)g_sim.width,
        .height = (int16_t)g_sim.height,
        .flush_cb = sim_display_flush,
        .user_data = NULL
    };

    ew_input_driver_t input_drv = {
        .poll_cb = sim_input_poll,
        .user_data = NULL
    };

    /* Build UI tree */
    sim_build_ui();

    /* Initialize Engine */
    ew_engine_init(&g_sim.engine, &g_sim.tree, &disp_drv, &input_drv, g_sim.slice_buffer, (int16_t)g_sim.slice_height);

    /* Render initial full frame */
    ew_engine_render_frame(&g_sim.engine);
    printf("[EmbedWeb Simulator] Initial frame rendered (%d slice flushes)\n", g_sim.flush_count);

    if (g_sim.test_mode) {
        printf("[EmbedWeb Simulator] Running automated interaction test...\n");
        /* Button 1 bounds: x=60 (20+40), y=110 (20+90), w=140, h=36 */
        int test_click_x = 80;
        int test_click_y = 125;

        /* Touch Down */
        queue_push(&g_sim.input_queue, (ew_input_event_t){ EW_EVENT_TOUCH_DOWN, (int16_t)test_click_x, (int16_t)test_click_y });
        ew_engine_process_input(&g_sim.engine);

        /* Touch Move */
        queue_push(&g_sim.input_queue, (ew_input_event_t){ EW_EVENT_TOUCH_MOVE, (int16_t)test_click_x, (int16_t)test_click_y });
        ew_engine_process_input(&g_sim.engine);

        /* Touch Up (inside button -> triggers click) */
        int flushes_before = g_sim.flush_count;
        queue_push(&g_sim.input_queue, (ew_input_event_t){ EW_EVENT_TOUCH_UP, (int16_t)test_click_x, (int16_t)test_click_y });
        ew_engine_process_input(&g_sim.engine);

        if (g_sim.current_val != 40) {
            fprintf(stderr, "FAIL: Expected progress 40, got %d\n", g_sim.current_val);
            return 1;
        }

        if (g_sim.flush_count <= flushes_before) {
            fprintf(stderr, "FAIL: Expected dirty region re-render flushes\n");
            return 1;
        }

        printf("PASS: Interaction test succeeded! Progress correctly incremented to 40%% with dirty redraw.\n");
        free(g_sim.slice_buffer);
        free(g_sim.framebuffer);
        return 0;
    }

#ifdef HAS_SDL2
    if (!g_sim.headless) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }

        SDL_Window* window = SDL_CreateWindow(
            "EmbedWeb Standalone Desktop Simulator",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            g_sim.width, g_sim.height,
            SDL_WINDOW_SHOWN
        );

        if (!window) {
            fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            renderer = SDL_CreateRenderer(window, -1, 0);
        }

        SDL_Texture* texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            g_sim.width, g_sim.height
        );

        bool running = true;
        SDL_Event event;
        int frame_count = 0;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        queue_push(&g_sim.input_queue, (ew_input_event_t){
                            EW_EVENT_TOUCH_DOWN, (int16_t)event.button.x, (int16_t)event.button.y
                        });
                    }
                } else if (event.type == SDL_MOUSEMOTION) {
                    if (event.motion.state & SDL_BUTTON_LMASK) {
                        queue_push(&g_sim.input_queue, (ew_input_event_t){
                            EW_EVENT_TOUCH_MOVE, (int16_t)event.motion.x, (int16_t)event.motion.y
                        });
                    }
                } else if (event.type == SDL_MOUSEBUTTONUP) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        queue_push(&g_sim.input_queue, (ew_input_event_t){
                            EW_EVENT_TOUCH_UP, (int16_t)event.button.x, (int16_t)event.button.y
                        });
                    }
                }
            }

            /* Process queued input */
            ew_engine_process_input(&g_sim.engine);

            /* Upload updated framebuffer to SDL Texture */
            SDL_UpdateTexture(texture, NULL, g_sim.framebuffer, g_sim.width * sizeof(uint16_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            frame_count++;
            if (test_frames > 0 && frame_count >= test_frames) {
                running = false;
            }

            SDL_Delay(16);
        }

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
#endif

    free(g_sim.slice_buffer);
    free(g_sim.framebuffer);
    printf("[EmbedWeb Simulator] Exited cleanly.\n");
    return 0;
}
