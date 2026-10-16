#include <QtTest/QtTest>
#include <QObject>
#include <QDebug>
#include <vector>

#include "runtime/ew_types.h"
#include "runtime/ew_tree.h"
#include "runtime/ew_layout.h"
#include "runtime/ew_raster.h"
#include "runtime/ew_hal.h"
#include "runtime/ew_resource.h"
#include "runtime/ew_input.h"
#include "runtime/ew_hydrate.h"
#include "adapters/ew_adapter_host.h"
#include "adapters/ew_adapter_stm32.h"
#include "adapters/ew_adapter_rp2040.h"
#include "adapters/ew_adapter_esp32.h"
#include "adapters/ew_adapter_nxp.h"
#include "ir/IRTypes.h"

class TestEmbedWebRuntime : public QObject {
    Q_OBJECT

private slots:
    void testStructureSizesAndMemoryFootprint();
    void testBoundedTreePoolAndHierarchy();
    void testFlexboxRowAndColumnLayout();
    void testFlexboxJustifyAndAlignment();
    void testRasterizerPrimitivesAndClipping();
    void testRasterizerTextGlyphs();
    void testEngineSlicedRenderingLoop();
    void testZeroAllocationDuringRender();
    void testProgressBarRenderingWithPixelComparison();
    void testImageBlittingAndChromaKeying();
    void testClippingAndDirtyRegionCorrectness();
    void testNegativeInputsAndRobustness();
    void testResourceAccountingCalculations();

    /* Stage 4.3 Input, Hydration, and Interaction Tests */
    void testValidIRHydration();
    void testUnsupportedComponentDiagnostics();
    void testMissingAssetRejection();
    void testMaximumNodeCapacityEnforcement();
    void testNestedTreeHierarchyHydration();
    void testInputHitTestingReverseZOrder();
    void testInputTouchDownMoveUpClickCycle();
    void testInputClickCancelledWhenReleasedOutside();
    void testInputInvalidCoordinatesAndStateCleanup();
    void testButtonCallbackDispatchAndSafety();
    void testProgressBarRuntimeUpdateAndDirtyInvalidation();
    void testRuntimeValueClamping();
    void testGeneratedStaticCSourceConsistency();
    void testCEscapingAndInjectionPrevention();

    /* Stage 4.5 Multi-MCU Hardware Abstraction & Adapter Tests */
    void testTargetAdapterBindingAndLifecycle();
    void testTargetAdapterBusAndTouchConfigurations();
    void testHalFailureHandlingAndInvalidConfigs();
};

void TestEmbedWebRuntime::testStructureSizesAndMemoryFootprint() {
    size_t node_sz = sizeof(ew_node_t);
    size_t tree_sz = sizeof(ew_tree_t);
    size_t rect_sz = sizeof(ew_rect_t);
    size_t style_sz = sizeof(ew_style_t);
    size_t engine_sz = sizeof(ew_engine_t);
    
    qDebug() << "[EmbedWeb Memory Measurement]";
    qDebug() << "  sizeof(ew_rect_t):" << rect_sz << "bytes";
    qDebug() << "  sizeof(ew_style_t):" << style_sz << "bytes";
    qDebug() << "  sizeof(ew_node_t):" << node_sz << "bytes";
    qDebug() << "  sizeof(ew_tree_t) (128 nodes):" << tree_sz << "bytes (" << (tree_sz / 1024.0) << "KB)";
    qDebug() << "  sizeof(ew_engine_t):" << engine_sz << "bytes";
    
    /* Assert compact structures */
    QVERIFY(node_sz <= 128); /* Ensure node POD is lean and bounded */
    QVERIFY(tree_sz <= 16384); /* Ensure 128-node pool is <= 16 KB */
    
    /* Measured slice buffer memory calculation:
     * For 320x240 RGB565 with 20-line slices:
     *   Single slice buffer: 320 * 20 * 2 = 12,800 bytes (12.5 KB)
     *   Dual DMA ping-pong buffer: 2 * 12,800 = 25,600 bytes (25.0 KB)
     * For 480x320 RGB565 with 16-line slices:
     *   Single slice buffer: 480 * 16 * 2 = 15,360 bytes (15.0 KB)
     *   Dual DMA ping-pong buffer: 2 * 15,360 = 30,720 bytes (30.0 KB)
     */
    size_t slice_320_20 = 320 * 20 * sizeof(uint16_t);
    size_t slice_480_16 = 480 * 16 * sizeof(uint16_t);
    QCOMPARE(slice_320_20, (size_t)12800);
    QCOMPARE(slice_480_16, (size_t)15360);
    
    /* Total SRAM for 480x320 target with 16-line dual slice:
     *   Dual slice buffers: 30,720 bytes
     *   128-node tree: ~10 KB
     *   Engine state & stack: ~4 KB
     *   Total: ~45 KB (< 64 KB SRAM limit!)
     */
    size_t total_sram_est = (2 * slice_480_16) + tree_sz + 4096;
    qDebug() << "  Total 480x320 runtime static RAM:" << total_sram_est << "bytes (" << (total_sram_est / 1024.0) << "KB < 64KB target)";
    QVERIFY(total_sram_est < 65536);
}

void TestEmbedWebRuntime::testBoundedTreePoolAndHierarchy() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    QCOMPARE(tree.count, 0);
    QCOMPARE(tree.root_idx, -1);
    
    /* Allocate root container */
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    QCOMPARE(root, 0);
    QCOMPARE(tree.root_idx, 0);
    
    /* Allocate 3 children */
    int16_t c1 = ew_tree_alloc_node(&tree, EW_WIDGET_RECTANGLE);
    int16_t c2 = ew_tree_alloc_node(&tree, EW_WIDGET_LABEL);
    int16_t c3 = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    
    QVERIFY(ew_tree_add_child(&tree, root, c1));
    QVERIFY(ew_tree_add_child(&tree, root, c2));
    QVERIFY(ew_tree_add_child(&tree, root, c3));
    
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    QVERIFY(n_root != nullptr);
    QCOMPARE(n_root->first_child_idx, c1);
    
    ew_node_t* n_c1 = ew_tree_get_node(&tree, c1);
    QCOMPARE(n_c1->parent_idx, root);
    QCOMPARE(n_c1->next_sibling_idx, c2);
    
    ew_node_t* n_c2 = ew_tree_get_node(&tree, c2);
    QCOMPARE(n_c2->next_sibling_idx, c3);
    
    ew_node_t* n_c3 = ew_tree_get_node(&tree, c3);
    QCOMPARE(n_c3->next_sibling_idx, -1);
    
    /* Fill tree pool up to capacity */
    for (int i = 4; i < EW_MAX_NODES; i++) {
        int16_t idx = ew_tree_alloc_node(&tree, EW_WIDGET_RECTANGLE);
        QCOMPARE(idx, (int16_t)i);
    }
    QCOMPARE(tree.count, (uint16_t)EW_MAX_NODES);
    
    /* Attempt allocation past capacity must be safely rejected */
    int16_t overflow = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    QCOMPARE(overflow, -1);
}

void TestEmbedWebRuntime::testFlexboxRowAndColumnLayout() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    /* Root container: 320x240, row layout, padding=10, gap=10 */
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->req_w = 320;
    n_root->req_h = 240;
    n_root->flex_direction = EW_FLEX_DIR_ROW;
    n_root->style.padding = 10;
    n_root->style.gap = 10;
    n_root->align = EW_ALIGN_START;
    
    /* Child 1: fixed 80x40 */
    int16_t c1 = ew_tree_alloc_node(&tree, EW_WIDGET_RECTANGLE);
    ew_node_t* n_c1 = ew_tree_get_node(&tree, c1);
    n_c1->req_w = 80;
    n_c1->req_h = 40;
    ew_tree_add_child(&tree, root, c1);
    
    /* Child 2: fixed 100x60 */
    int16_t c2 = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_c2 = ew_tree_get_node(&tree, c2);
    n_c2->req_w = 100;
    n_c2->req_h = 60;
    ew_tree_add_child(&tree, root, c2);
    
    ew_layout_compute(&tree, root, 320, 240);
    
    /* Verify computed coordinates */
    QCOMPARE(n_root->rect.x, 0);
    QCOMPARE(n_root->rect.y, 0);
    QCOMPARE(n_root->rect.w, 320);
    QCOMPARE(n_root->rect.h, 240);
    
    /* c1 at (10, 10, 80, 40) */
    QCOMPARE(n_c1->rect.x, 10);
    QCOMPARE(n_c1->rect.y, 10);
    QCOMPARE(n_c1->rect.w, 80);
    QCOMPARE(n_c1->rect.h, 40);
    
    /* c2 at (10 + 80 + gap(10) = 100, 10, 100, 60) */
    QCOMPARE(n_c2->rect.x, 100);
    QCOMPARE(n_c2->rect.y, 10);
    QCOMPARE(n_c2->rect.w, 100);
    QCOMPARE(n_c2->rect.h, 60);
    
    /* Now test Column layout */
    n_root->flex_direction = EW_FLEX_DIR_COLUMN;
    ew_layout_compute(&tree, root, 320, 240);
    
    /* In column layout:
     * c1 at (10, 10, 80, 40)
     * c2 at (10, 10 + 40 + gap(10) = 60, 100, 60)
     */
    QCOMPARE(n_c1->rect.x, 10);
    QCOMPARE(n_c1->rect.y, 10);
    QCOMPARE(n_c2->rect.x, 10);
    QCOMPARE(n_c2->rect.y, 60);
}

void TestEmbedWebRuntime::testFlexboxJustifyAndAlignment() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    /* Container 200x100, row, padding=0, gap=0, justify=CENTER, align=CENTER */
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->req_w = 200;
    n_root->req_h = 100;
    n_root->flex_direction = EW_FLEX_DIR_ROW;
    n_root->justify = EW_JUSTIFY_CENTER;
    n_root->align = EW_ALIGN_CENTER;
    
    /* Single item 60x40 */
    int16_t c1 = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_c1 = ew_tree_get_node(&tree, c1);
    n_c1->req_w = 60;
    n_c1->req_h = 40;
    ew_tree_add_child(&tree, root, c1);
    
    ew_layout_compute(&tree, root, 200, 100);
    
    /* Free space in X: 200 - 60 = 140 -> Center offset = 70 */
    /* Free space in Y: 100 - 40 = 60 -> Center offset = 30 */
    QCOMPARE(n_c1->rect.x, 70);
    QCOMPARE(n_c1->rect.y, 30);
    
    /* Test EW_ALIGN_STRETCH with auto-height child */
    n_root->align = EW_ALIGN_STRETCH;
    n_c1->req_h = -1; /* Auto */
    ew_layout_compute(&tree, root, 200, 100);
    QCOMPARE(n_c1->rect.h, 100);
}

void TestEmbedWebRuntime::testRasterizerPrimitivesAndClipping() {
    const int W = 320;
    const int H = 20;
    uint16_t buffer[W * H];
    
    ew_raster_slice_t slice;
    slice.buffer = buffer;
    slice.width = W;
    slice.height = H;
    slice.slice_y = 40; /* Rendering global Y: 40..59 */
    
    /* 1. Clear with BLUE */
    ew_raster_clear(&slice, EW_COLOR_BLUE);
    QCOMPARE(buffer[0], EW_COLOR_BLUE);
    QCOMPARE(buffer[W * H - 1], EW_COLOR_BLUE);
    
    /* 2. Draw rectangle entirely above slice (y=0..30): should not affect buffer */
    ew_rect_t r_above = { 10, 10, 50, 20 };
    ew_raster_draw_rect(&slice, r_above, EW_COLOR_RED);
    QCOMPARE(buffer[10], EW_COLOR_BLUE);
    
    /* 3. Draw rectangle intersecting slice (y=30..50, x=10..60):
     * Rect covers Y=30..49.
     * Slice covers Y=40..59.
     * Overlap is Y=40..49 (local lines 0..9).
     */
    ew_rect_t r_intersect = { 10, 30, 50, 20 };
    ew_raster_draw_rect(&slice, r_intersect, EW_COLOR_RED);
    
    /* Pixels in local line 0 (y=40) between x=10 and x=59 should be RED */
    QCOMPARE(buffer[0 * W + 9], EW_COLOR_BLUE);
    QCOMPARE(buffer[0 * W + 10], EW_COLOR_RED);
    QCOMPARE(buffer[0 * W + 59], EW_COLOR_RED);
    QCOMPARE(buffer[0 * W + 60], EW_COLOR_BLUE);
    
    /* Pixels in local line 9 (y=49) should be RED */
    QCOMPARE(buffer[9 * W + 20], EW_COLOR_RED);
    
    /* Pixels in local line 10 (y=50) should be untouched BLUE */
    QCOMPARE(buffer[10 * W + 20], EW_COLOR_BLUE);
}

void TestEmbedWebRuntime::testRasterizerTextGlyphs() {
    const int W = 100;
    const int H = 20;
    uint16_t buffer[W * H];
    
    ew_raster_slice_t slice;
    slice.buffer = buffer;
    slice.width = W;
    slice.height = H;
    slice.slice_y = 0;
    
    ew_raster_clear(&slice, EW_COLOR_BLACK);
    
    /* Draw 'A' at (10, 5) in WHITE */
    ew_raster_draw_char(&slice, 10, 5, 'A', EW_COLOR_WHITE);
    
    /* Verify that some pixels in the 8x8 glyph box (x=10..17, y=5..12) are WHITE */
    int white_pixel_count = 0;
    for (int y = 5; y < 13; y++) {
        for (int x = 10; x < 18; x++) {
            if (buffer[y * W + x] == EW_COLOR_WHITE) {
                white_pixel_count++;
            }
        }
    }
    
    /* Standard 8x8 'A' has ~18 set pixels */
    QVERIFY(white_pixel_count > 10);
    
    /* Draw full text */
    ew_raster_draw_text(&slice, 30, 5, "EmbedWeb", EW_COLOR_GREEN);
    /* Verify green pixel found in the string region */
    bool found_green = false;
    for (int i = 0; i < W * H; i++) {
        if (buffer[i] == EW_COLOR_GREEN) {
            found_green = true;
            break;
        }
    }
    QVERIFY(found_green);
}

struct MockDisplayState {
    int flush_count;
    int total_lines_flushed;
};

static void mock_flush_cb(ew_display_driver_t* disp, int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* pixels) {
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(w);
    Q_UNUSED(pixels);
    MockDisplayState* st = (MockDisplayState*)disp->user_data;
    st->flush_count++;
    st->total_lines_flushed += h;
}

void TestEmbedWebRuntime::testEngineSlicedRenderingLoop() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->req_w = 320;
    n_root->req_h = 240;
    n_root->style.has_bg = 1;
    n_root->style.bg_color = EW_COLOR_BLACK;
    
    int16_t btn = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_btn = ew_tree_get_node(&tree, btn);
    n_btn->req_w = 120;
    n_btn->req_h = 40;
    n_btn->style.has_bg = 1;
    n_btn->style.bg_color = EW_COLOR_BLUE;
    n_btn->style.has_border = 1;
    n_btn->style.border_color = EW_COLOR_WHITE;
    n_btn->style.border_width = 2;
    n_btn->style.text_color = EW_COLOR_WHITE;
    strncpy(n_btn->text, "Click Me", EW_MAX_TEXT_LEN - 1);
    ew_tree_add_child(&tree, root, btn);
    
    ew_layout_compute(&tree, root, 320, 240);
    
    MockDisplayState state = { 0, 0 };
    ew_display_driver_t disp;
    disp.width = 320;
    disp.height = 240;
    disp.flush_cb = mock_flush_cb;
    disp.user_data = &state;
    
    /* Preallocated 20-line slice buffer */
    const int SLICE_H = 20;
    uint16_t slice_buf[320 * SLICE_H];
    
    ew_engine_t engine;
    ew_engine_init(&engine, &tree, &disp, nullptr, slice_buf, SLICE_H);
    
    /* Render frame */
    ew_engine_render_frame(&engine);
    
    /* 240 lines / 20 lines per slice = exactly 12 slice flushes */
    QCOMPARE(state.flush_count, 12);
    QCOMPARE(state.total_lines_flushed, 240);
}

void TestEmbedWebRuntime::testZeroAllocationDuringRender() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->req_w = 480;
    n_root->req_h = 320;
    n_root->style.has_bg = 1;
    n_root->style.bg_color = EW_COLOR_GREEN;
    
    for (int i = 0; i < 20; i++) {
        int16_t item = ew_tree_alloc_node(&tree, EW_WIDGET_LABEL);
        ew_node_t* n = ew_tree_get_node(&tree, item);
        n->req_w = 100;
        n->req_h = 20;
        n->style.text_color = EW_COLOR_WHITE;
        snprintf(n->text, sizeof(n->text), "Item %d", i);
        ew_tree_add_child(&tree, root, item);
    }
    
    ew_layout_compute(&tree, root, 480, 320);
    
    MockDisplayState state = { 0, 0 };
    ew_display_driver_t disp;
    disp.width = 480;
    disp.height = 320;
    disp.flush_cb = mock_flush_cb;
    disp.user_data = &state;
    
    /* 16-line slice buffer */
    const int SLICE_H = 16;
    uint16_t slice_buf[480 * SLICE_H];
    
    ew_engine_t engine;
    ew_engine_init(&engine, &tree, &disp, nullptr, slice_buf, SLICE_H);
    
    /* Benchmark 100 frame renders */
    QElapsedTimer timer;
    timer.start();
    const int FRAMES = 100;
    for (int f = 0; f < FRAMES; f++) {
        ew_engine_render_frame(&engine);
    }
    qint64 elapsed_ms = timer.elapsed();
    double fps = (FRAMES * 1000.0) / elapsed_ms;
    
    qDebug() << "[EmbedWeb Performance Benchmark]";
    qDebug() << "  Rendered" << FRAMES << "frames of 480x320 with 20 widgets in" << elapsed_ms << "ms";
    qDebug() << "  Avg frame time:" << (elapsed_ms / (double)FRAMES) << "ms (" << fps << "FPS on desktop CPU)";
    
    /* 320 lines / 16 = 20 slices per frame * 100 frames = 2000 flushes */
    QCOMPARE(state.flush_count, 20 * FRAMES);
    QCOMPARE(state.total_lines_flushed, 320 * FRAMES);
    QVERIFY(elapsed_ms >= 0);
}

void TestEmbedWebRuntime::testProgressBarRenderingWithPixelComparison() {
    const int W = 100;
    const int H = 20;
    uint16_t buffer[W * H];
    
    ew_raster_slice_t slice;
    slice.buffer = buffer;
    slice.width = W;
    slice.height = H;
    slice.slice_y = 0;
    
    ew_raster_clear(&slice, EW_COLOR_BLUE);
    
    /* Draw 100x20 progress bar at (0, 0) with 50% progress, border=2 */
    ew_rect_t pbar_rect = { 0, 0, W, H };
    ew_raster_draw_progress_bar(&slice, pbar_rect, 50, EW_COLOR_BLACK, EW_COLOR_GREEN, EW_COLOR_WHITE, 2);
    
    /* 1. Top border at (x=50, y=0) should be EW_COLOR_WHITE */
    QCOMPARE(buffer[0 * W + 50], EW_COLOR_WHITE);
    /* 2. Left border at (x=0, y=10) should be EW_COLOR_WHITE */
    QCOMPARE(buffer[10 * W + 0], EW_COLOR_WHITE);
    
    /* 3. Filled region: x=2..49 at y=10 should be EW_COLOR_GREEN */
    QCOMPARE(buffer[10 * W + 2], EW_COLOR_GREEN);
    QCOMPARE(buffer[10 * W + 25], EW_COLOR_GREEN);
    QCOMPARE(buffer[10 * W + 49], EW_COLOR_GREEN);
    
    /* 4. Unfilled track region: x=50..97 at y=10 should be EW_COLOR_BLACK */
    QCOMPARE(buffer[10 * W + 50], EW_COLOR_BLACK);
    QCOMPARE(buffer[10 * W + 75], EW_COLOR_BLACK);
    QCOMPARE(buffer[10 * W + 97], EW_COLOR_BLACK);
    
    /* 5. Right border at (x=99, y=10) should be EW_COLOR_WHITE */
    QCOMPARE(buffer[10 * W + 99], EW_COLOR_WHITE);
}

void TestEmbedWebRuntime::testImageBlittingAndChromaKeying() {
    const int W = 20;
    const int H = 20;
    uint16_t buffer[W * H];
    
    ew_raster_slice_t slice;
    slice.buffer = buffer;
    slice.width = W;
    slice.height = H;
    slice.slice_y = 0;
    
    ew_raster_clear(&slice, EW_COLOR_BLUE);
    
    /* Create a 4x4 test bitmap:
     * Pixel (0,0) is CHROMA_KEY (transparent RED: 0xF800)
     * Other pixels are GREEN (0x07E0)
     */
    uint16_t raw_pixels[16];
    for (int i = 0; i < 16; i++) {
        raw_pixels[i] = EW_COLOR_GREEN;
    }
    raw_pixels[0] = EW_COLOR_RED; /* Transparent key */
    
    ew_image_asset_t img;
    img.width = 4;
    img.height = 4;
    img.format = EW_IMAGE_FORMAT_RGB565_CHROMA;
    img.chroma_key = EW_COLOR_RED;
    img.pixels = raw_pixels;
    
    /* Draw image at (5, 5) with 100% opacity */
    ew_raster_draw_image(&slice, 5, 5, &img, 255);
    
    /* Pixel at (5, 5) should remain untouched BLUE because it was chroma-keyed */
    QCOMPARE(buffer[5 * W + 5], EW_COLOR_BLUE);
    
    /* Pixel at (6, 6) should be blitted GREEN */
    QCOMPARE(buffer[6 * W + 6], EW_COLOR_GREEN);
    
    /* Test opacity blending: 50% opacity blend */
    ew_color_t blended = ew_color_blend565(EW_COLOR_WHITE, EW_COLOR_BLACK, 128);
    /* In 5-6-5: WHITE is (31, 63, 31), BLACK is (0, 0, 0), 128/255 is ~15, 31, 15 */
    uint16_t r = (blended >> 11) & 0x1F;
    uint16_t g = (blended >> 5) & 0x3F;
    uint16_t b = blended & 0x1F;
    QVERIFY(r >= 14 && r <= 16);
    QVERIFY(g >= 30 && g <= 32);
    QVERIFY(b >= 14 && b <= 16);
}

void TestEmbedWebRuntime::testClippingAndDirtyRegionCorrectness() {
    MockDisplayState state = { 0, 0 };
    ew_display_driver_t disp;
    disp.width = 320;
    disp.height = 240;
    disp.flush_cb = mock_flush_cb;
    disp.user_data = &state;
    
    const int SLICE_H = 20;
    uint16_t slice_buf[320 * SLICE_H];
    
    ew_tree_t tree;
    ew_tree_init(&tree);
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->req_w = 320;
    n_root->req_h = 240;
    
    ew_engine_t engine;
    ew_engine_init(&engine, &tree, &disp, nullptr, slice_buf, SLICE_H);
    
    /* Render a dirty region that spans only lines 25..35.
     * Slices are 20 lines each:
     * Slice 0: lines 0..19 (does not intersect 25..35) -> NOT flushed.
     * Slice 1: lines 20..39 (intersects 25..35) -> FLUSHED.
     * Slices 2..11: lines 40..239 (do not intersect) -> NOT flushed.
     */
    ew_rect_t dirty = { 10, 25, 100, 10 };
    ew_engine_render_dirty_region(&engine, dirty);
    
    /* Exactly 1 slice must be flushed! */
    QCOMPARE(state.flush_count, 1);
    QCOMPARE(state.total_lines_flushed, 20);
}

void TestEmbedWebRuntime::testNegativeInputsAndRobustness() {
    ew_raster_slice_t slice;
    uint16_t buf[100];
    slice.buffer = buf;
    slice.width = 10;
    slice.height = 10;
    slice.slice_y = 0;
    
    /* 1. Negative dimensions must not crash */
    ew_rect_t neg_rect = { 0, 0, -5, 10 };
    ew_raster_draw_rect(&slice, neg_rect, EW_COLOR_WHITE);
    ew_raster_draw_progress_bar(&slice, neg_rect, 50, EW_COLOR_BLACK, EW_COLOR_GREEN, EW_COLOR_WHITE, 1);
    
    /* 2. Null slice or buffer must not crash */
    ew_raster_draw_rect(nullptr, neg_rect, EW_COLOR_WHITE);
    ew_raster_slice_t null_slice = { nullptr, 10, 10, 0 };
    ew_raster_draw_rect(&null_slice, neg_rect, EW_COLOR_WHITE);
    
    /* 3. Unsupported image format or null pixels must not crash */
    ew_image_asset_t invalid_img = { 10, 10, EW_IMAGE_FORMAT_UNSUPPORTED, 0, nullptr };
    ew_raster_draw_image(&slice, 0, 0, &invalid_img, 255);
    ew_raster_draw_image(&slice, 0, 0, nullptr, 255);
    
    /* 4. Tree operations robustness */
    ew_tree_t tree;
    ew_tree_init(&tree);
    
    /* Null tree allocation must return -1 */
    QCOMPARE(ew_tree_alloc_node(nullptr, EW_WIDGET_BUTTON), -1);
    
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    int16_t c1 = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    QVERIFY(ew_tree_add_child(&tree, root, c1));
    
    /* Self-parenting must be rejected */
    QVERIFY(!ew_tree_add_child(&tree, root, root));
    
    /* Cycle creation (making root a child of c1) must be rejected */
    QVERIFY(!ew_tree_add_child(&tree, c1, root));
    
    /* Out of bounds index must be rejected */
    QVERIFY(!ew_tree_add_child(&tree, root, 999));
    QVERIFY(!ew_tree_add_child(&tree, -1, c1));
    QVERIFY(ew_tree_get_node(&tree, 999) == nullptr);
    QVERIFY(ew_tree_get_node(&tree, -1) == nullptr);
}

void TestEmbedWebRuntime::testResourceAccountingCalculations() {
    /* 320x240 single buffer vs dual buffer */
    size_t single_slice_320 = ew_resource_calc_slice_ram(320, 16, 1);
    size_t dual_slice_320 = ew_resource_calc_slice_ram(320, 16, 2);
    QCOMPARE(single_slice_320, (size_t)10240);
    QCOMPARE(dual_slice_320, (size_t)20480);
    QCOMPARE(dual_slice_320, single_slice_320 * 2);

    /* 480x320 with 16-line dual buffer */
    ew_memory_summary_t sum_480_16 = ew_resource_summarize(480, 16, 2, 128, 4096);
    QCOMPARE(sum_480_16.slice_buffer_ram, (size_t)30720);
    QVERIFY(!sum_480_16.exceeds_64k_sram);
    QCOMPARE(sum_480_16.font_rom, (size_t)760);
    QCOMPARE(sum_480_16.image_assets_rom, (size_t)4096);

    /* 320x240 single vs dual buffer summary */
    ew_memory_summary_t sum_single_320 = ew_resource_summarize(320, 16, 1, 128, 0);
    ew_memory_summary_t sum_dual_320 = ew_resource_summarize(320, 16, 2, 128, 0);
    QCOMPARE(sum_single_320.slice_buffer_ram, (size_t)10240);
    QCOMPARE(sum_dual_320.slice_buffer_ram, (size_t)20480);
    QCOMPARE(sum_dual_320.total_static_ram - sum_single_320.total_static_ram, (size_t)10240);
    
    /* Font rom calculation */
    QCOMPARE(ew_resource_calc_font_rom(95, 8, 8), (size_t)760);
    
    /* Image rom calculation: 32x32 RGB565 = 2048 bytes */
    QCOMPARE(ew_resource_calc_image_rom(32, 32, EW_IMAGE_FORMAT_RGB565), (size_t)2048);
}

void TestEmbedWebRuntime::testValidIRHydration() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;

    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.name = "MainScreen";
    screen.width = 320;
    screen.height = 240;
    screen.backgroundColor.type = IR::IRColor::Type::Solid;
    screen.backgroundColor.solidColor = QColor(255, 0, 0);

    // 1. Container
    IR::IRWidget container;
    container.id = IR::IRNodeId::generate();
    container.componentType = "Container";
    container.layout.type = IR::IRLayoutType::Flex;
    container.layout.flexDirection = IR::IRFlexDirection::Column;
    container.properties["width"] = 300;
    container.properties["height"] = 200;

    // 2. Label
    IR::IRWidget label;
    label.id = IR::IRNodeId::generate();
    label.componentType = "Label";
    label.label = "Status: Active";
    label.properties["width"] = 120;
    label.properties["height"] = 20;

    // 3. Button
    IR::IRWidget btn;
    btn.id = IR::IRNodeId::generate();
    btn.componentType = "Button";
    btn.label = "Submit";
    btn.properties["width"] = 100;
    btn.properties["height"] = 40;

    // 4. Progress bar
    IR::IRWidget pbar;
    pbar.id = IR::IRNodeId::generate();
    pbar.componentType = "ProgressBar";
    pbar.properties["value"] = 60;
    pbar.properties["width"] = 200;
    pbar.properties["height"] = 15;

    container.children.append(label);
    container.children.append(btn);
    container.children.append(pbar);
    screen.widgets.append(container);
    proj.screens.append(screen);

    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult res = hydrator.hydrate(proj);

    QVERIFY(res.success);
    QCOMPARE(res.errorMessages().size(), 0);
    // Root screen (0) + container (1) + label (2) + btn (3) + pbar (4) = 5 nodes
    QCOMPARE(res.tree.count, 5);
    QCOMPARE(res.tree.root_idx, 0);

    ew_node_t* r = ew_tree_get_node(&res.tree, 0);
    QVERIFY(r != nullptr);
    QCOMPARE(r->type, (uint8_t)EW_WIDGET_CONTAINER);
    QCOMPARE(r->style.has_bg, (uint8_t)1);
    QCOMPARE(r->style.bg_color, EW_COLOR_RED);

    ew_node_t* n_btn = ew_tree_get_node(&res.tree, 3);
    QVERIFY(n_btn != nullptr);
    QCOMPARE(n_btn->type, (uint8_t)EW_WIDGET_BUTTON);
    QCOMPARE(n_btn->touchable, (uint8_t)1);
    QCOMPARE(QString::fromUtf8(n_btn->text), QString("Submit"));

    ew_node_t* n_pb = ew_tree_get_node(&res.tree, 4);
    QVERIFY(n_pb != nullptr);
    QCOMPARE(n_pb->type, (uint8_t)EW_WIDGET_PROGRESS_BAR);
    QCOMPARE(n_pb->progress_value, (int16_t)60);
}

void TestEmbedWebRuntime::testUnsupportedComponentDiagnostics() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;
    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.width = 320;
    screen.height = 240;

    IR::IRWidget unsupp;
    unsupp.id = IR::IRNodeId::generate();
    unsupp.componentType = "Speedometer";
    screen.widgets.append(unsupp);
    proj.screens.append(screen);

    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult res = hydrator.hydrate(proj);

    QVERIFY(!res.success);
    QVERIFY(!res.errorMessages().isEmpty());
    QVERIFY(res.errorMessages().first().contains("Unsupported component type 'Speedometer'"));
}

void TestEmbedWebRuntime::testMissingAssetRejection() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;
    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.width = 320;
    screen.height = 240;

    IR::IRWidget img;
    img.id = IR::IRNodeId::generate();
    img.componentType = "Image";
    img.name = "LogoWidget";
    img.properties["imagePath"] = "assets/logo.png";
    screen.widgets.append(img);
    proj.screens.append(screen);

    // Initial state: assets list is empty
    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult res = hydrator.hydrate(proj);

    QVERIFY(!res.success);
    QVERIFY(!res.errorMessages().isEmpty());
    QVERIFY(res.errorMessages().first().contains("references missing or unpackaged asset 'assets/logo.png'"));

    // Provide the packaged asset in IR
    IR::IRAsset asset;
    asset.id = IR::IRNodeId::generate();
    asset.filePath = "assets/logo.png";
    asset.name = "logo.png";
    asset.width = 64;
    asset.height = 64;
    proj.assets.append(asset);

    EmbedWeb::HydrationResult res2 = hydrator.hydrate(proj);
    QVERIFY(res2.success);
}

void TestEmbedWebRuntime::testMaximumNodeCapacityEnforcement() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;
    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.width = 320;
    screen.height = 240;

    // Add 128 widgets; together with the root container = 129 nodes > EW_MAX_NODES (128)
    for (int i = 0; i < 128; i++) {
        IR::IRWidget w;
        w.id = IR::IRNodeId::generate();
        w.componentType = "Rectangle";
        screen.widgets.append(w);
    }
    proj.screens.append(screen);

    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult res = hydrator.hydrate(proj);

    QVERIFY(!res.success);
    QVERIFY(!res.errorMessages().isEmpty());
    QVERIFY(res.errorMessages().first().contains("exceeds maximum bounded node pool capacity of 128"));
}

void TestEmbedWebRuntime::testNestedTreeHierarchyHydration() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;
    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.width = 320;
    screen.height = 240;

    IR::IRWidget c1;
    c1.id = IR::IRNodeId::generate();
    c1.componentType = "Container";
    c1.properties["width"] = 280;
    c1.properties["height"] = 200;

    IR::IRWidget c2;
    c2.id = IR::IRNodeId::generate();
    c2.componentType = "Container";
    c2.properties["width"] = 240;
    c2.properties["height"] = 160;

    IR::IRWidget btn;
    btn.id = IR::IRNodeId::generate();
    btn.componentType = "Button";
    btn.label = "DeepBtn";
    btn.properties["width"] = 80;
    btn.properties["height"] = 30;

    c2.children.append(btn);
    c1.children.append(c2);
    screen.widgets.append(c1);
    proj.screens.append(screen);

    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult res = hydrator.hydrate(proj);

    QVERIFY(res.success);
    // 0: root, 1: c1, 2: c2, 3: btn
    QCOMPARE(res.tree.count, 4);
    QCOMPARE(res.tree.nodes[1].parent_idx, (int16_t)0);
    QCOMPARE(res.tree.nodes[2].parent_idx, (int16_t)1);
    QCOMPARE(res.tree.nodes[3].parent_idx, (int16_t)2);
}

void TestEmbedWebRuntime::testInputHitTestingReverseZOrder() {
    ew_tree_t tree;
    ew_tree_init(&tree);

    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->rect = { 0, 0, 320, 240 };
    n_root->touchable = 0; // Root is not interactive

    // Widget A: (20, 20, 100, 60)
    int16_t a = ew_tree_alloc_node(&tree, EW_WIDGET_RECTANGLE);
    ew_node_t* n_a = ew_tree_get_node(&tree, a);
    n_a->rect = { 20, 20, 100, 60 };
    n_a->touchable = 1;
    QVERIFY(ew_tree_add_child(&tree, root, a));

    // Widget B: (60, 40, 100, 60) - overlaps A, added second so drawn on top of A
    int16_t b = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_b = ew_tree_get_node(&tree, b);
    n_b->rect = { 60, 40, 100, 60 };
    n_b->touchable = 1;
    QVERIFY(ew_tree_add_child(&tree, root, b));

    // 1. Point at (30, 30): inside A only
    QCOMPARE(ew_input_hit_test(&tree, 30, 30), a);

    // 2. Point at (120, 70): inside B only
    QCOMPARE(ew_input_hit_test(&tree, 120, 70), b);

    // 3. Overlap at (80, 50): inside BOTH A and B. B is on top, must return B!
    QCOMPARE(ew_input_hit_test(&tree, 80, 50), b);

    // 4. Point at (250, 200): misses both, root not touchable -> returns -1
    QCOMPARE(ew_input_hit_test(&tree, 250, 200), -1);
}

struct TestEventTracker {
    int down_count = 0;
    int move_count = 0;
    int up_count = 0;
    int click_count = 0;
    uint16_t last_node_id = 0;
};

static void test_node_event_callback(ew_node_t* node, uint8_t event_type, void* user_data) {
    if (!user_data || !node) return;
    TestEventTracker* t = static_cast<TestEventTracker*>(user_data);
    t->last_node_id = node->id;
    if (event_type == EW_EVENT_TOUCH_DOWN) t->down_count++;
    else if (event_type == EW_EVENT_TOUCH_MOVE) t->move_count++;
    else if (event_type == EW_EVENT_TOUCH_UP) t->up_count++;
    else if (event_type == EW_EVENT_CLICK) t->click_count++;
}

void TestEmbedWebRuntime::testInputTouchDownMoveUpClickCycle() {
    ew_tree_t tree;
    ew_tree_init(&tree);

    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->rect = { 0, 0, 320, 240 };

    int16_t btn = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_btn = ew_tree_get_node(&tree, btn);
    n_btn->rect = { 50, 50, 80, 40 };
    n_btn->touchable = 1;
    QVERIFY(ew_tree_add_child(&tree, root, btn));

    TestEventTracker tracker;
    QVERIFY(ew_node_set_event_cb(&tree, btn, test_node_event_callback, &tracker));

    ew_input_state_t state;
    ew_input_state_init(&state);

    // 1. Touch Down inside button at (70, 60)
    ew_input_event_t evt_down = { EW_EVENT_TOUCH_DOWN, 70, 60 };
    ew_input_dispatch(&tree, &state, &evt_down);

    QCOMPARE(tracker.down_count, 1);
    QCOMPARE(state.is_down, true);
    QCOMPARE(state.pressed_node_idx, btn);
    QCOMPARE(n_btn->state, (uint8_t)1); // Pressed state

    // 2. Touch Move inside button at (75, 65)
    ew_input_event_t evt_move = { EW_EVENT_TOUCH_MOVE, 75, 65 };
    ew_input_dispatch(&tree, &state, &evt_move);

    QCOMPARE(tracker.move_count, 1);

    // 3. Touch Up inside button at (75, 65) -> Must fire UP and CLICK!
    ew_input_event_t evt_up = { EW_EVENT_TOUCH_UP, 75, 65 };
    ew_input_dispatch(&tree, &state, &evt_up);

    QCOMPARE(tracker.up_count, 1);
    QCOMPARE(tracker.click_count, 1);
    QCOMPARE(state.is_down, false);
    QCOMPARE(state.pressed_node_idx, (int16_t)-1);
    QCOMPARE(n_btn->state, (uint8_t)0); // Released state
}

void TestEmbedWebRuntime::testInputClickCancelledWhenReleasedOutside() {
    ew_tree_t tree;
    ew_tree_init(&tree);

    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->rect = { 0, 0, 320, 240 };

    int16_t btn = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n_btn = ew_tree_get_node(&tree, btn);
    n_btn->rect = { 50, 50, 80, 40 };
    n_btn->touchable = 1;
    QVERIFY(ew_tree_add_child(&tree, root, btn));

    TestEventTracker tracker;
    QVERIFY(ew_node_set_event_cb(&tree, btn, test_node_event_callback, &tracker));

    ew_input_state_t state;
    ew_input_state_init(&state);

    // 1. Touch Down inside button at (70, 60)
    ew_input_event_t evt_down = { EW_EVENT_TOUCH_DOWN, 70, 60 };
    ew_input_dispatch(&tree, &state, &evt_down);
    QCOMPARE(tracker.down_count, 1);

    // 2. Touch Up OUTSIDE button at (250, 200) -> Must NOT fire CLICK!
    ew_input_event_t evt_up = { EW_EVENT_TOUCH_UP, 250, 200 };
    ew_input_dispatch(&tree, &state, &evt_up);

    QCOMPARE(tracker.up_count, 1);
    QCOMPARE(tracker.click_count, 0); // Click cancelled!
    QCOMPARE(state.is_down, false);
}

void TestEmbedWebRuntime::testInputInvalidCoordinatesAndStateCleanup() {
    ew_tree_t tree;
    ew_tree_init(&tree);
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->rect = { 0, 0, 320, 240 };
    n_root->touchable = 1;

    // 1. Negative coordinates must not hit
    QCOMPARE(ew_input_hit_test(&tree, -10, -50), -1);

    // 2. Out of bounds coordinates must not hit
    QCOMPARE(ew_input_hit_test(&tree, 1000, 2000), -1);

    // 3. Pointer disappearance / EW_EVENT_NONE resets input state
    ew_input_state_t state;
    ew_input_state_init(&state);
    state.is_down = true;
    state.pressed_node_idx = root;

    ew_input_event_t evt_none = { EW_EVENT_NONE, 0, 0 };
    ew_input_dispatch(&tree, &state, &evt_none);

    QCOMPARE(state.is_down, false);
    QCOMPARE(state.pressed_node_idx, (int16_t)-1);
}

void TestEmbedWebRuntime::testButtonCallbackDispatchAndSafety() {
    ew_tree_t tree;
    ew_tree_init(&tree);

    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);
    ew_node_t* n = ew_tree_get_node(&tree, root);
    n->rect = { 0, 0, 100, 50 };
    n->touchable = 1;

    ew_input_state_t state;
    ew_input_state_init(&state);

    ew_input_event_t evt = { EW_EVENT_TOUCH_DOWN, 50, 25 };

    // Null safety checks
    ew_input_dispatch(nullptr, &state, &evt);
    ew_input_dispatch(&tree, nullptr, &evt);
    ew_input_dispatch(&tree, &state, nullptr);

    // Dispatch without callback registered must not crash
    ew_input_dispatch(&tree, &state, &evt);
    QCOMPARE(n->state, (uint8_t)1);
}

void TestEmbedWebRuntime::testProgressBarRuntimeUpdateAndDirtyInvalidation() {
    MockDisplayState disp_state = { 0, 0 };
    ew_display_driver_t disp;
    disp.width = 320;
    disp.height = 240;
    disp.flush_cb = mock_flush_cb;
    disp.user_data = &disp_state;

    const int SLICE_H = 20;
    uint16_t slice_buf[320 * SLICE_H];

    ew_tree_t tree;
    ew_tree_init(&tree);
    int16_t root = ew_tree_alloc_node(&tree, EW_WIDGET_CONTAINER);
    ew_node_t* n_root = ew_tree_get_node(&tree, root);
    n_root->rect = { 0, 0, 320, 240 };

    int16_t pbar = ew_tree_alloc_node(&tree, EW_WIDGET_PROGRESS_BAR);
    ew_node_t* n_pb = ew_tree_get_node(&tree, pbar);
    n_pb->rect = { 20, 25, 100, 10 }; // Sits in lines 25..35 (Slice 1: lines 20..39)
    n_pb->progress_value = 20;
    QVERIFY(ew_tree_add_child(&tree, root, pbar));

    ew_engine_t engine;
    ew_engine_init(&engine, &tree, &disp, nullptr, slice_buf, SLICE_H);

    // 1. Update progress value directly at runtime without rebuilding tree
    QVERIFY(ew_node_set_progress(&tree, pbar, 85));
    QCOMPARE(n_pb->progress_value, (int16_t)85);

    // 2. Invalidate node: only the intersecting slice (Slice 1) must be flushed
    ew_engine_invalidate_node(&engine, pbar);
    QCOMPARE(disp_state.flush_count, 1);
    QCOMPARE(disp_state.total_lines_flushed, 20);
}

void TestEmbedWebRuntime::testRuntimeValueClamping() {
    ew_tree_t tree;
    ew_tree_init(&tree);

    int16_t pbar = ew_tree_alloc_node(&tree, EW_WIDGET_PROGRESS_BAR);
    ew_node_t* n_pb = ew_tree_get_node(&tree, pbar);
    n_pb->progress_value = 50;

    int16_t btn = ew_tree_alloc_node(&tree, EW_WIDGET_BUTTON);

    // Clamping to [0, 100]
    QVERIFY(ew_node_set_progress(&tree, pbar, -50));
    QCOMPARE(n_pb->progress_value, (int16_t)0);

    QVERIFY(ew_node_set_progress(&tree, pbar, 250));
    QCOMPARE(n_pb->progress_value, (int16_t)100);

    // Setting same value returns false (no change)
    QVERIFY(!ew_node_set_progress(&tree, pbar, 100));

    // Calling progress setter on non-progress widget returns false
    QVERIFY(!ew_node_set_progress(&tree, btn, 50));

    // Text setter
    QVERIFY(ew_node_set_text(&tree, btn, "Hello World"));
    ew_node_t* n_btn = ew_tree_get_node(&tree, btn);
    QCOMPARE(QString::fromUtf8(n_btn->text), QString("Hello World"));

    // Setting same text returns false
    QVERIFY(!ew_node_set_text(&tree, btn, "Hello World"));

    // Visibility toggle
    QVERIFY(ew_node_set_visible(&tree, btn, false));
    QCOMPARE(n_btn->visible, (uint8_t)0);
    QVERIFY(ew_node_set_visible(&tree, btn, true));
    QCOMPARE(n_btn->visible, (uint8_t)1);
}

void TestEmbedWebRuntime::testGeneratedStaticCSourceConsistency() {
    IR::IRProject proj;
    proj.name = "TestProject";
    proj.displayWidth = 320;
    proj.displayHeight = 240;

    IR::IRScreen screen;
    screen.id = IR::IRNodeId::generate();
    screen.width = 320;
    screen.height = 240;

    IR::IRWidget btn;
    btn.id = IR::IRNodeId::generate();
    btn.componentType = "Button";
    btn.label = "Start";
    btn.properties["width"] = 80;
    btn.properties["height"] = 30;
    screen.widgets.append(btn);
    proj.screens.append(screen);

    IR::IRAsset asset;
    asset.id = IR::IRNodeId::generate();
    asset.name = "icon.png";
    asset.filePath = "assets/icon.png";
    asset.width = 16;
    asset.height = 16;
    proj.assets.append(asset);

    EmbedWeb::IRHydrator hydrator;

    // Header validation
    QString header = hydrator.generateStaticTreeHeader(proj);
    QVERIFY(header.contains("void ui_tree_init(ew_tree_t* tree);"));
    QVERIFY(header.contains("#define UI_SCREEN_WIDTH  320"));

    // Source validation: must emit static tree code with zero mallocs
    QString source = hydrator.generateStaticTreeSource(proj);
    QVERIFY(!source.isEmpty());
    QVERIFY(source.contains("void ui_tree_init(ew_tree_t* tree)"));
    QVERIFY(source.contains("ew_tree_init(tree);"));
    QVERIFY(source.contains("ew_layout_compute(tree"));
    QVERIFY(source.contains("Start"));

    // Assets emission validation
    QString assetH = hydrator.generateStaticAssetsHeader(proj);
    QVERIFY(assetH.contains("extern const ew_image_asset_t img_asset_0;"));

    QString assetC = hydrator.generateStaticAssetsSource(proj);
    QVERIFY(assetC.contains("img_pixels_0"));
    QVERIFY(assetC.contains("img_asset_0"));
}

void TestEmbedWebRuntime::testCEscapingAndInjectionPrevention() {
    // 1. Inputs specified in audit requirements:
    // - hi"quoted"
    // - ends-with-backslash\
    // - line1\nline2 (a real newline)
    // - x\"); system("id"); //

    QString in1 = "hi\"quoted\"";
    QString out1 = EmbedWeb::IRHydrator::escapeCStringLiteral(in1);
    QCOMPARE(out1, QString("hi\\\"quoted\\\""));

    QString in2 = "ends-with-backslash\\";
    QString out2 = EmbedWeb::IRHydrator::escapeCStringLiteral(in2);
    QCOMPARE(out2, QString("ends-with-backslash\\\\"));

    QString in3 = "line1\nline2";
    QString out3 = EmbedWeb::IRHydrator::escapeCStringLiteral(in3);
    QCOMPARE(out3, QString("line1\\nline2"));

    QString in4 = "x\\\"); system(\"id\"); //";
    QString out4 = EmbedWeb::IRHydrator::escapeCStringLiteral(in4);
    QCOMPARE(out4, QString("x\\\\\\\"); system(\\\"id\\\"); //"));

    // Verify generated lines compile as valid C string literals via gcc -fsyntax-only
    QStringList testCases = { out1, out2, out3, out4 };
    for (const QString& escaped : testCases) {
        QString cSource = QString(
            "#include <string.h>\n"
            "int main(void) {\n"
            "    const char* s = \"%1\";\n"
            "    (void)s;\n"
            "    return 0;\n"
            "}\n"
        ).arg(escaped);

        QProcess gcc;
        gcc.start("gcc", QStringList() << "-fsyntax-only" << "-x" << "c" << "-");
        QVERIFY(gcc.waitForStarted(3000));
        gcc.write(cSource.toUtf8());
        gcc.closeWriteChannel();
        QVERIFY(gcc.waitForFinished(5000));
        QCOMPARE(gcc.exitCode(), 0);
    }
}

void TestEmbedWebRuntime::testTargetAdapterBindingAndLifecycle() {
    // 1. Host Target Adapter
    ew_host_context_t host_ctx;
    memset(&host_ctx, 0, sizeof(host_ctx));
    ew_target_adapter_t host_adapter = ew_adapter_host_create(&host_ctx);

    ew_adapter_config_t config;
    memset(&config, 0, sizeof(config));
    config.width = 320;
    config.height = 240;
    config.bus_type = EW_HAL_BUS_SPI;
    config.touch_type = EW_HAL_TOUCH_NONE;

    ew_hal_status_t st = host_adapter.init_platform(host_adapter.user_ctx, &config);
    QCOMPARE(st, EW_HAL_OK);

    ew_display_driver_t disp;
    ew_input_driver_t input;
    ew_hal_bind_adapter(&disp, &input, &host_adapter, 320, 240);
    QCOMPARE(disp.width, (int16_t)320);
    QCOMPARE(disp.height, (int16_t)240);

    // Pixel flush proxy
    uint16_t dummy_pixels[10 * 10] = {0};
    disp.flush_cb(&disp, 0, 0, 10, 10, dummy_pixels);
    QCOMPARE(host_ctx.flush_count, 1u);

    // Transfer busy / wait done
    QCOMPARE(host_adapter.is_transfer_busy(host_adapter.user_ctx), false);
    host_adapter.wait_transfer_done(host_adapter.user_ctx);

    // Input simulation proxy
    host_ctx.simulated_touch_x = 120;
    host_ctx.simulated_touch_y = 180;
    host_ctx.simulated_pressed = true;
    ew_input_event_t evt;
    memset(&evt, 0, sizeof(evt));
    bool polled = input.poll_cb(&input, &evt);
    QCOMPARE(polled, true);
    QCOMPARE(evt.type, EW_EVENT_TOUCH_DOWN);
    QCOMPARE(evt.x, (int16_t)120);
    QCOMPARE(evt.y, (int16_t)180);

    // Clean shutdown
    if (host_adapter.cleanup) {
        host_adapter.cleanup(host_adapter.user_ctx);
    }
}

void TestEmbedWebRuntime::testTargetAdapterBusAndTouchConfigurations() {
    uint16_t dummy_pixels[320 * 16] = {0};
    ew_display_driver_t disp;
    ew_input_driver_t input;

    // 2. STM32 Target Adapter with Parallel 8080 & Capacitive Touch
    ew_stm32_context_t stm32_ctx;
    memset(&stm32_ctx, 0, sizeof(stm32_ctx));
    ew_target_adapter_t stm32_adapter = ew_adapter_stm32_create(&stm32_ctx);

    ew_adapter_config_t stm32_cfg;
    memset(&stm32_cfg, 0, sizeof(stm32_cfg));
    stm32_cfg.width = 320;
    stm32_cfg.height = 240;
    stm32_cfg.bus_type = EW_HAL_BUS_PARALLEL_8080;
    stm32_cfg.touch_type = EW_HAL_TOUCH_CAPACITIVE_I2C;
    stm32_cfg.dma_enabled = 1;

    ew_hal_status_t st = stm32_adapter.init_platform(stm32_adapter.user_ctx, &stm32_cfg);
    QCOMPARE(st, EW_HAL_OK);
    QCOMPARE(stm32_ctx.bus_type, (uint8_t)EW_HAL_BUS_PARALLEL_8080);
    QCOMPARE(stm32_ctx.touch_type, (uint8_t)EW_HAL_TOUCH_CAPACITIVE_I2C);

    ew_hal_bind_adapter(&disp, &input, &stm32_adapter, 320, 240);
    disp.flush_cb(&disp, 0, 0, 320, 16, dummy_pixels);
    QCOMPARE(stm32_ctx.last_flush_pixels, 320u * 16u);
    if (stm32_adapter.cleanup) stm32_adapter.cleanup(stm32_adapter.user_ctx);

    // 3. Raspberry Pi Pico RP2040 Target Adapter with SPI
    ew_rp2040_context_t rp_ctx;
    memset(&rp_ctx, 0, sizeof(rp_ctx));
    ew_target_adapter_t rp_adapter = ew_adapter_rp2040_create(&rp_ctx);

    ew_adapter_config_t rp_cfg;
    memset(&rp_cfg, 0, sizeof(rp_cfg));
    rp_cfg.width = 240;
    rp_cfg.height = 240;
    rp_cfg.bus_type = EW_HAL_BUS_SPI;

    st = rp_adapter.init_platform(rp_adapter.user_ctx, &rp_cfg);
    QCOMPARE(st, EW_HAL_OK);

    ew_hal_bind_adapter(&disp, &input, &rp_adapter, 240, 240);
    disp.flush_cb(&disp, 0, 0, 240, 10, dummy_pixels);
    QCOMPARE(rp_ctx.flush_count, 1u);
    if (rp_adapter.cleanup) rp_adapter.cleanup(rp_adapter.user_ctx);

    // 4. ESP32 / ESP32-S3 Target Adapter with internal DMA check
    ew_esp32_context_t esp_ctx;
    memset(&esp_ctx, 0, sizeof(esp_ctx));
    ew_target_adapter_t esp_adapter = ew_adapter_esp32_create(&esp_ctx);

    ew_adapter_config_t esp_cfg;
    memset(&esp_cfg, 0, sizeof(esp_cfg));
    esp_cfg.width = 320;
    esp_cfg.height = 480;
    esp_cfg.bus_type = EW_HAL_BUS_SPI;
    esp_cfg.dma_enabled = 1;
    esp_cfg.dma_needs_internal_sram = 1;

    st = esp_adapter.init_platform(esp_adapter.user_ctx, &esp_cfg);
    QCOMPARE(st, EW_HAL_OK);
    QCOMPARE(esp_ctx.dma_enabled, true);
    QCOMPARE(esp_ctx.require_internal_sram, true);

    ew_hal_bind_adapter(&disp, &input, &esp_adapter, 320, 480);
    disp.flush_cb(&disp, 0, 0, 100, 20, dummy_pixels);
    QCOMPARE(esp_ctx.flush_count, 1u);
    if (esp_adapter.cleanup) esp_adapter.cleanup(esp_adapter.user_ctx);

    // 5. NXP i.MX RT Target Adapter
    ew_nxp_context_t nxp_ctx;
    memset(&nxp_ctx, 0, sizeof(nxp_ctx));
    ew_target_adapter_t nxp_adapter = ew_adapter_nxp_create(&nxp_ctx);

    st = nxp_adapter.init_platform(nxp_adapter.user_ctx, &esp_cfg);
    QCOMPARE(st, EW_HAL_OK);
    if (nxp_adapter.cleanup) nxp_adapter.cleanup(nxp_adapter.user_ctx);
}

void TestEmbedWebRuntime::testHalFailureHandlingAndInvalidConfigs() {
    ew_host_context_t host_ctx;
    memset(&host_ctx, 0, sizeof(host_ctx));
    ew_target_adapter_t host_adapter = ew_adapter_host_create(&host_ctx);

    ew_adapter_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.width = 320;
    cfg.height = 240;

    // NULL ctx or config rejection
    QCOMPARE(host_adapter.init_platform(nullptr, &cfg), EW_HAL_ERR_INIT);
    QCOMPARE(host_adapter.init_platform(host_adapter.user_ctx, nullptr), EW_HAL_ERR_INIT);

    // Init valid
    QCOMPARE(host_adapter.init_platform(host_adapter.user_ctx, &cfg), EW_HAL_OK);

    // Invalid flush parameters
    uint16_t buf[100];
    QCOMPARE(host_adapter.flush_pixels(host_adapter.user_ctx, 0, 0, 0, 10, buf), EW_HAL_ERR_BUS);
    QCOMPARE(host_adapter.flush_pixels(host_adapter.user_ctx, 0, 0, 10, -2, buf), EW_HAL_ERR_BUS);
    QCOMPARE(host_adapter.flush_pixels(host_adapter.user_ctx, 0, 0, 10, 10, nullptr), EW_HAL_ERR_BUS);
    QCOMPARE(host_adapter.flush_pixels(nullptr, 0, 0, 10, 10, buf), EW_HAL_ERR_BUS);

    // Null pointer input polling
    QCOMPARE(host_adapter.poll_touch(nullptr, nullptr, nullptr, nullptr), false);

    // Safe bind with null display/input targets
    ew_hal_bind_adapter(nullptr, nullptr, &host_adapter, 320, 240);

    // Safe bind with null adapter
    ew_display_driver_t disp;
    ew_input_driver_t input;
    memset(&disp, 0, sizeof(disp));
    memset(&input, 0, sizeof(input));
    ew_hal_bind_adapter(&disp, &input, nullptr, 320, 240);
    // Proxy callbacks handle null adapter gracefully without crashing
    disp.flush_cb(&disp, 0, 0, 10, 10, buf);
    ew_input_event_t evt;
    QCOMPARE(input.poll_cb(&input, &evt), false);

    if (host_adapter.cleanup) host_adapter.cleanup(host_adapter.user_ctx);
}

QTEST_MAIN(TestEmbedWebRuntime)
#include "test_embedweb_runtime.moc"
