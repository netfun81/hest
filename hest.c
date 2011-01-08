#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#define LENGTH(array) (sizeof(array) / sizeof(array[0]))

typedef unsigned char HestWindow;

static HestWindow curwin = 0;
static HestWindow top = 0;
static Display *dpy;
static GC gc;
static Window pager;
static Window root;
static short screen;
static unsigned short sh;
static unsigned short sw;
static HestWindow stack[40];
static Window windows[40];
static KeySym keys[40] = {
    XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8,     XK_9,      XK_0,
    XK_q, XK_w, XK_e, XK_r, XK_t, XK_y, XK_u, XK_i,     XK_o,      XK_p,
    XK_a, XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k,     XK_l,      XK_semicolon,
    XK_z, XK_x, XK_c, XK_v, XK_b, XK_n, XK_m, XK_comma, XK_period, XK_slash
};
static const char *menu_cmd[]            = {"dmenu_run", NULL};
static const char *term_cmd[]            = {"urxvt", NULL};
static const char *current_border_color  = "#bbccff";
static const char *fg_color              = "#ffeedd";
static const char *normal_border_color   = "#445566";
static const char *occupied_bg_color     = "#223344";
static const char *vacant_bg_color       = "#000000";

static void
drawpager(void) {
    char buffer[256];
    Colormap cmap = DefaultColormap(dpy, screen);
    XColor color;
    HestWindow i;
    unsigned short x, y, h, w;
    time_t t;
    struct tm *tmp;

    w = (sw/10 - 12);
    h = (sh/10 - 12);

    XSetForeground(dpy, gc, XBlackPixel(dpy, screen));
    XFillRectangle(dpy, pager, gc, 0, 0, sw/2.5, sh/2.5 + 32 + 1);

    for(i = 0; i < LENGTH(windows); ++i) {
        x = (i % 10 + 1) * 10 + i % 10 * w;
        y = (i / 10 + 1) * 10 + i / 10 * h;

        strcpy(buffer, XKeysymToString(keys[i]));

        if(windows[i])
            XAllocNamedColor(dpy, cmap, occupied_bg_color, &color, &color);
        else
            XAllocNamedColor(dpy, cmap, vacant_bg_color, &color, &color);

        XSetForeground(dpy, gc, color.pixel);
        XFillRectangle(dpy, pager, gc, x, y, w, h);

        if(i == curwin)
            XAllocNamedColor(dpy, cmap, current_border_color, &color, &color);
        else
            XAllocNamedColor(dpy, cmap, normal_border_color, &color, &color);

        XSetForeground(dpy, gc, color.pixel);
        XDrawRectangle(dpy, pager, gc, x, y, w, h);

        XAllocNamedColor(dpy, cmap, fg_color, &color, &color);
        XSetForeground(dpy, gc, color.pixel);
        XDrawString(dpy, pager, gc, x + 8, y + 16, buffer, strlen(buffer));
    }

    t = time(NULL);
    tmp = localtime(&t);
    strftime(buffer, LENGTH(buffer), "%Y-%m-%d %H:%M:%S", tmp);
    XDrawString(dpy, pager, gc, 16, sh/2.5 + 16, buffer, strlen(buffer));
}

static void
showhide(void) {
    HestWindow i;

    if(windows[curwin]) {
        XMapRaised(dpy, windows[curwin]);
        XMoveResizeWindow(dpy, windows[curwin], 0, 0, sw, sh);
        XSetInputFocus(dpy, windows[curwin], RevertToPointerRoot, CurrentTime);
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    }

    for(i = 0; i < LENGTH(windows); ++i)
        if(windows[i] && i != curwin)
            XUnmapWindow(dpy, windows[i]);
}

static void
swap(HestWindow a, HestWindow b) {
    Window temp;

    temp = windows[a];
    windows[a] = windows[b];
    windows[b] = temp;

    stack[top = 0] = curwin;
    showhide();
}

static void
spawn(const char *argv[]) {
    pid_t pid;

    if(!(pid = fork())) {
        close(ConnectionNumber(dpy));
        setsid();
        execvp(argv[0], (char**)argv);
        fprintf(stderr, "Couldn't execvp.\n");
        exit(EXIT_FAILURE);
    }
}

static void
view(HestWindow window) {
    stack[top = 0] = curwin = window;
    showhide();
}

static void
configurenotify(XEvent *ev) {
    XWindowAttributes wa;

    ev = ev;

    XGetWindowAttributes(dpy, root, &wa);

    sw = wa.width;
    sh = wa.height;

    XMoveResizeWindow(dpy, windows[curwin], 0, 0, sw, sh);
}

static void
destroynotify(XEvent *ev) {
    HestWindow i;
    XDestroyWindowEvent *dwe = &ev->xdestroywindow;

    for(i = 0; i < LENGTH(windows); ++i)
        if(windows[i] == dwe->window) {
            memset(&windows[i], '\0', sizeof(Window));

            if(i == curwin && top)
                curwin = stack[--top];

            showhide();
            return;
        }
}

static void
keypress(XEvent *ev) {
    unsigned long i;
    KeySym keysym;
    XKeyEvent *kev = &ev->xkey;

    keysym = XKeycodeToKeysym(dpy, (KeyCode)kev->keycode, 0);

    if((kev->state & ControlMask || keysym == XK_Control_L || keysym == XK_Control_R) &&
       (kev->state & Mod4Mask || keysym == XK_Super_L || keysym == XK_Super_R)) {
        XMapRaised(dpy, pager);
        drawpager();
    }

    if(kev->state & Mod4Mask) {
        if(keysym == XK_Return)
            if(kev->state & ShiftMask)
                spawn(term_cmd);
            else
                spawn(menu_cmd);
        else
            for(i = 0; i < LENGTH(keys); ++i)
                if(keys[i] == keysym) {
                    if(kev->state & ShiftMask)
                        swap(i, curwin);
                    else if(i != curwin)
                        view(i);

                    return;
                }
    }
}

static void
keyrelease(XEvent *ev) {
    KeySym keysym;
    XKeyEvent *kev = &ev->xkey;

    keysym = XKeycodeToKeysym(dpy, (KeyCode)kev->keycode, 0);

    if(keysym == XK_Control_L || keysym == XK_Control_R ||
       keysym == XK_Super_L || keysym == XK_Super_R)
        XUnmapWindow(dpy, pager);
}

static void
maprequest(XEvent *ev) {
    static XWindowAttributes wa;
    XMapRequestEvent *mrev = &ev->xmaprequest;
    HestWindow i;

    if(!XGetWindowAttributes(dpy, mrev->window, &wa))
        return;

    for(i = 0; i < LENGTH(windows); ++i)
        if(windows[i] == mrev->window) {
            showhide();
            return;
        }

    if(windows[curwin])
        for(curwin = 0; curwin < LENGTH(windows) && windows[curwin]; ++curwin);

    stack[++top] = curwin;
    windows[curwin] = mrev->window;
    showhide();
}

static int
xerror(Display *dpy, XErrorEvent *ee) {
    fprintf(stderr, "Got an XErrorEvent: %p, %p\n", (void *)dpy, (void *)ee);

    return 0;
}

static void
setup(void) {
    unsigned char i;
    static const KeySym modifiers[] = {
        Mod4Mask,
        Mod4Mask | ShiftMask,
        Mod4Mask | ShiftMask | ControlMask,
        Mod4Mask |             ControlMask,
        Mod4Mask                           | Mod5Mask,
        Mod4Mask | ShiftMask               | Mod5Mask,
        Mod4Mask | ShiftMask | ControlMask | Mod5Mask,
        Mod4Mask |             ControlMask | Mod5Mask,
    };
    XWindowAttributes attributes;

    signal(SIGCHLD, SIG_IGN);

    memset(&windows, 0, sizeof(windows));

    if(!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    XSetErrorHandler(xerror);
    XGetWindowAttributes(dpy, root, &attributes);
    sw = attributes.width;
    sh = attributes.height;

    pager = XCreateSimpleWindow(dpy, root, 0, sh,
                                sw, sh/2.5 + 32 + 1, 0,
                                WhitePixel(dpy, screen),
                                BlackPixel(dpy, screen));

    XMoveResizeWindow(dpy, pager, 0, sh - sh/2.5 - 32, sw, sh/2.5 + 32 + 1);

    gc = XCreateGC(dpy, pager, 0, NULL);

    XSelectInput(dpy, root, KeyPressMask | SubstructureNotifyMask | SubstructureRedirectMask);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Super_L), AnyModifier, root, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Super_R), AnyModifier, root, False, GrabModeAsync, GrabModeAsync);
    for(i = 0; i < LENGTH(modifiers); ++i)
        XGrabKey(dpy, AnyKey, modifiers[i], root, True, GrabModeAsync, GrabModeAsync);
}

int
main(void) {
    static void(*handler[LASTEvent])(XEvent *) = {
        [ConfigureNotify] = configurenotify,
        [DestroyNotify] = destroynotify,
        [KeyPress] = keypress,
        [KeyRelease] = keyrelease,
        [MapRequest] = maprequest
    };
    XEvent ev;

    setup();

    while(!XNextEvent(dpy, &ev))
        if(handler[ev.type])
            handler[ev.type](&ev);

    return EXIT_SUCCESS;
}
