#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>

#define LENGTH(array)   (sizeof(array) / sizeof(array[0]))

static void destroynotify(XEvent*);
static int findvacancy(void);
static void keypress(XEvent*);
static void maprequest(XEvent*);
static void run(void);
static void setup(void);
static void showhide(void);
static void swap(int, int);
static void spawn(const char**);
static void view(int);
static int xerror(Display*, XErrorEvent*);

static Display* dpy;
static Window root;
static Window windows[40];
static int current_window = 0;
static void(*handler[LASTEvent])(XEvent*) = {
    [DestroyNotify] = destroynotify,
    [KeyPress] = keypress,
    [MapRequest] = maprequest
};
static int keys[40] = {
    XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8,     XK_9,      XK_0,
    XK_q, XK_w, XK_e, XK_r, XK_t, XK_y, XK_u, XK_i,     XK_o,      XK_p, 
    XK_a, XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k,     XK_l,      XK_semicolon,
    XK_z, XK_x, XK_c, XK_v, XK_b, XK_n, XK_m, XK_comma, XK_period, XK_slash
};
static int last_window = 0;
static const char* menu_cmd[] = {"dmenu_run", NULL};
static int screen;
static int screen_height;
static int screen_width;
static const char* term_cmd[] = {"urxvt", NULL};

void
destroynotify(XEvent* ev) {
    int i;
    XDestroyWindowEvent* dwe = &ev->xdestroywindow;

    for(i = 0; i < LENGTH(windows); ++i)
        if(windows[i] == dwe->window)
            memset(&windows[i], 0, sizeof(Window));

    view(last_window);
}

int
findvacancy(void) {
    int i;

    for(i = 0; i < LENGTH(windows) && windows[i]; ++i);
    return i;
}

void
keypress(XEvent* ev) {
    int i;
    KeySym keysym;
    XKeyEvent* kev = &ev->xkey;

    keysym = XKeycodeToKeysym(dpy, (KeyCode)kev->keycode, 0);
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
                        swap(current_window, i);
                    else
                        view(i);
                    return;
                }
    }
}

void
maprequest(XEvent* ev) {
    static XWindowAttributes wa;
    XMapRequestEvent* mrev = &ev->xmaprequest;

    if(!XGetWindowAttributes(dpy, mrev->window, &wa))
        return;

    XMapWindow(dpy, mrev->window);
    if(!windows[current_window])
        windows[current_window] = mrev->window;
    else
        if((current_window = findvacancy()) != -1)
            windows[current_window] = mrev->window;
    showhide();
}

void
run(void) {
    XEvent ev;

    while(!XNextEvent(dpy, &ev))
        if(handler[ev.type])
            handler[ev.type](&ev);
}

void
setup(void) {
    int i;
    static const int modifiers[] = {Mod4Mask, Mod4Mask | ShiftMask};
    XWindowAttributes attributes;

    memset(&windows, 0, sizeof(windows));

    if(!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    XSetErrorHandler(xerror);
    XGetWindowAttributes(dpy, root, &attributes);
    screen_width = attributes.width;
    screen_height = attributes.height;

    XSelectInput(dpy, root, KeyPressMask | SubstructureNotifyMask | SubstructureRedirectMask);

    for(i = 0; i < LENGTH(modifiers); ++i)
        XGrabKey(dpy, AnyKey, modifiers[i], root, True, GrabModeAsync, GrabModeAsync);
}

void
showhide(void) {
    int i;

    if(windows[current_window]) {
        XMoveResizeWindow(dpy, windows[current_window], 0, 0, screen_width, screen_height);
        XRaiseWindow(dpy, windows[current_window]);
        XSetInputFocus(dpy, windows[current_window], RevertToPointerRoot, CurrentTime);
    }

    for(i = 0; i < LENGTH(windows); ++i)
        if(windows[i] && i != current_window)
            XMoveResizeWindow(dpy, windows[i], 0, screen_height, screen_width, screen_height);
}

void
swap(int a, int b) {
    Window temp = windows[a];

    windows[a] = windows[b];
    windows[b] = temp;
    showhide();
}

void
spawn(const char* argv[]) {
    int pid;

    if(!(pid = fork())) {
        close(ConnectionNumber(dpy));
        setsid();
        execvp(argv[0], (char**)argv);
        fprintf(stderr, "Couldn't execvp.\n");
        exit(0);
    }
}

void
view(int window) {
    current_window = last_window = window;
    showhide();
}

int
xerror(Display* dpy, XErrorEvent* ee) {
    fprintf(stderr, "Got an XErrorEvent.\n");
    return 0;
}

int
main(void) {
    setup();
    run();

    return EXIT_SUCCESS;
}
