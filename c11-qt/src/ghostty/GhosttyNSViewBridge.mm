#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include "GhosttyNSViewBridge.h"

void *ghostty_bridge_create_child_nsview(void *parentNSView)
{
    NSView *parent = (__bridge NSView *)parentNSView;

    NSView *child = [[NSView alloc] initWithFrame:parent.bounds];
    // Do NOT set wantsLayer here. Ghostty's Metal renderer sets
    // view.layer = CAMetalLayer BEFORE view.wantsLayer = true,
    // making it a "layer-hosting" view. Pre-setting wantsLayer
    // creates a default CALayer and breaks this sequence.
    child.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    [parent addSubview:child];

    return (__bridge_retained void *)child;
}

void ghostty_bridge_resize_child_nsview(void *childNSView, double width, double height)
{
    NSView *child = (__bridge NSView *)childNSView;
    child.frame = NSMakeRect(0, 0, width, height);
}

void ghostty_bridge_destroy_child_nsview(void *childNSView)
{
    NSView *child = (__bridge_transfer NSView *)childNSView;
    [child removeFromSuperview];
}

void ghostty_bridge_make_window_opaque(void *nsview)
{
    NSView *view = (__bridge NSView *)nsview;
    NSWindow *window = view.window;
    if (!window) return;
    window.opaque = YES;
    window.hasShadow = YES;
}

void ghostty_bridge_debug_child_nsview(void *childNSView)
{
    NSView *child = (__bridge NSView *)childNSView;
    NSLog(@"[c11-bridge] child wantsLayer=%d layer=%@ frame=%@",
          child.wantsLayer,
          child.layer ? NSStringFromClass([child.layer class]) : @"nil",
          NSStringFromRect(child.frame));
}
