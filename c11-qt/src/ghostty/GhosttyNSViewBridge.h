#pragma once

// Objective-C++ bridge for creating a child NSView inside a Qt widget's
// native view. Ghostty renders via Metal into this child view, avoiding
// conflicts with Qt's own layer management.

#ifdef __cplusplus
extern "C" {
#endif

// Creates a layer-backed child NSView inside the parent NSView (from QWidget::winId()).
// Returns the child NSView pointer suitable for ghostty_platform_macos_s.nsview.
void *ghostty_bridge_create_child_nsview(void *parentNSView);

// Resizes the child NSView to fill its parent.
void ghostty_bridge_resize_child_nsview(void *childNSView, double width, double height);

// Removes and releases the child NSView.
void ghostty_bridge_destroy_child_nsview(void *childNSView);

// Debug: log child NSView state.
void ghostty_bridge_debug_child_nsview(void *childNSView);

// Make the NSView's window opaque and non-transparent.
void ghostty_bridge_make_window_opaque(void *nsview);

#ifdef __cplusplus
}
#endif
