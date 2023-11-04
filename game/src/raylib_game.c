#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"


#define PI 3.14159265358979323846
#define THETA_MIN 0.0
#define THETA_MAX (8.0 * PI)
#define PERIOD THETA_MAX
#define LINE_SPACING  (1.5 / 12.0)
#define LINE_LENGTH (LINE_SPACING / 2.0)
#define G_RATE  (1.0 / (2.0 * PI))
#define G_FACTOR (G_RATE / 3.0)
#define SPEED (PERIOD * 1.5)


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#include "raylib_game.h"
#endif


typedef struct Point2d
{
    float x;
    float y;
}
Point2d;


typedef struct Point3d
{
    float x;
    float y;
    float z;
}
Point3d;


typedef struct Line
{
    Point3d start;
    Point3d end;
    float alpha;
}
Line;


typedef struct SpiralCamera
{
    float y_screen_offset;
    float x_screen_offset;
    float screen_scale;
    float scale;
    float y_camera;
    float z_camera;
    float width;
    float height;
}
SpiralCamera;


void new_camera(SpiralCamera* camera, float width, float height, float scale, float y_pos,float z_pos)
{
    (*camera).y_screen_offset = height / 2.0;
    (*camera).x_screen_offset = width / 2.0;
    (*camera).screen_scale=max(width, height) / scale;
    (*camera).scale = scale;
    (*camera).y_camera=y_pos;
    (*camera).z_camera=z_pos;
    (*camera).width = width;
    (*camera).height = height;
}


void resize(SpiralCamera* camera, float width, float height)
{
    if ((fabs((*camera).width - width) + fabs((*camera).height - height)) < 1.0)
    {
        return;
    }
    (*camera).y_screen_offset = height / 2.0;
    (*camera).x_screen_offset = width / 2.0;
    (*camera).screen_scale = max(width,height) / (*camera).scale;
}


Point2d project(SpiralCamera* camera, Point3d point)
{
    Point2d tmp;
    tmp.x = (*camera).x_screen_offset + (*camera).screen_scale * (point.x / (point.z - (*camera).z_camera));
    tmp.y = (*camera).y_screen_offset + (*camera).screen_scale * ((point.y - (*camera).y_camera) / (point.z - (*camera).z_camera));
    return tmp;
}


void draw_new(Line* line, SpiralCamera* camera, Color color)
{
    Point2d start = project(camera, (*line).start);
    Point2d end = project(camera, (*line).end);
    Color newColor = { color.r, color.g, color.b, (unsigned char)((*line).alpha * 255.0) };
    DrawLineEx((Vector2) { start.x, start.y}, (Vector2) { end.x, end.y}, 2.0, newColor);
}


typedef struct Spiral
{
    Color foreground;
    float angle_offset;
    float factor;
    float offset;
    Line* segment;
    int capacity;
    int segment_count;
}
Spiral;


float d_theta(float theta, float l_line_length, float rate, float factor)
{
    float inv_theta = sqrt(rate * rate + factor * factor * theta * theta);
    return (l_line_length / inv_theta);
}


Point3d get_point(float theta, float factor, float angle_offset, float rate)
{
    float theta_angle = theta + angle_offset;
    Point3d tmp;
    tmp.x = theta * factor * cos(theta_angle);
    tmp.y = rate * theta;
    tmp.z = -theta * factor * sin(theta_angle);
    return tmp;
}


float get_alpha(Point3d point, float factor, float rate)
{
    return min(
        1.0,
        atan((point.y * factor / rate * 0.1 + 0.02 - point.z) * 40.0) * 0.35 + 0.65
    );
}


void render(
    Spiral* spiral,
    SpiralCamera* camera
    )
{
    for (int i = 0; i < (*spiral).segment_count; i++)
    {
        draw_new((*spiral).segment+i, camera, (*spiral).foreground);
    }
}


void new_spiral(Spiral* spiral, Color foreground, float angle_offset,float  factor)
{
        angle_offset = angle_offset * PI;
        factor = factor * G_FACTOR;
        
        (*spiral).foreground = foreground;
        (*spiral).angle_offset = angle_offset;
        (*spiral).factor = factor;
        (*spiral).offset = 0.0;
        (*spiral).segment =NULL;
        (*spiral).segment_count = 0;
        (*spiral).capacity = 0;
}


void push_back(Spiral* spiral,Line line )
{
    if((*spiral).segment == NULL)
    {
        (*spiral).segment = (Line*)malloc(sizeof(Line));
        (*spiral).capacity = 1;
        
    }
    else if ((*spiral).capacity < (*spiral).segment_count + 1)
    {
        (*spiral).capacity *= 2;
        Line* new_segment = (Line*)malloc((*spiral).capacity*sizeof(Line));
        memcpy(new_segment, (*spiral).segment, (*spiral).segment_count*sizeof(Line));
        free((*spiral).segment);
        (*spiral).segment = new_segment;
    }
    (*spiral).segment[(*spiral).segment_count] = line;
    (*spiral).segment_count += 1;

}


void compute_segment(Spiral* spiral, float dt)
{
    (*spiral).offset += dt * SPEED;
    if((*spiral).offset > PERIOD)
    {
       (*spiral).offset -= PERIOD;
    }

    float theta = THETA_MIN
        - d_theta(
            THETA_MIN,
            LINE_SPACING * (*spiral).offset / PERIOD,
            G_RATE,
            (*spiral).factor
        );

    (*spiral).segment_count = 0;

    while( theta < THETA_MAX)
    {
        float theta_old = max(theta, THETA_MIN);
        theta += d_theta(theta, LINE_LENGTH, G_RATE, (*spiral).factor);
        float theta_end = min(
            (theta_old + max(theta, THETA_MIN)) / 2.0,
            THETA_MAX,
        );

        if (theta_end < THETA_MIN)
        {
            continue;
        }

        Point3d start = get_point(theta_old, (*spiral).factor, (*spiral).angle_offset, G_RATE);
        Point3d end = get_point(theta_end, (*spiral).factor, (*spiral).angle_offset, G_RATE);
        float alpha = get_alpha(start, (*spiral).factor, G_RATE);

        Line line = { start, end, alpha };
        push_back(spiral, line);
    }
}


void process_key_left(int* active_channel, int* active_spiral)
{
    if (IsKeyPressed(KEY_LEFT))
    {
        *active_channel -= 1;
        if (*active_channel <0)
        {
            *active_channel = 2;
            *active_spiral -= 1;
            if (*active_spiral < 0)
            {
                *active_spiral = 5;
            }
        }
    }
}


void process_key_right(int* active_channel, int* active_spiral)
{
    if (IsKeyPressed(KEY_RIGHT))
    {
        *active_channel += 1;
        if (*active_channel > 2)
        {
            *active_channel = 0;
            *active_spiral += 1;
            if (*active_spiral > 5)
            {
                *active_spiral = 0;
            }
        }
    }
}


void process_key_up(int* active_channel, Spiral* spirals, int* active_spiral)
{
    if (IsKeyDown(KEY_UP))
    {
        switch (*active_channel)
        {
        case 0:
            (*(spirals + *active_spiral)).foreground.r += 1;
            if ((*(spirals + *active_spiral)).foreground.r == 0)
            {
                (*(spirals + *active_spiral)).foreground.r = 255;
            }
            break;

        case 1:
            (*(spirals + *active_spiral)).foreground.g += 1;
            if ((*(spirals + *active_spiral)).foreground.g == 0)
            {
                (*(spirals + *active_spiral)).foreground.g = 255;
            }
            break;

        case 2:
            (*(spirals + *active_spiral)).foreground.b += 1;
            if ((*(spirals + *active_spiral)).foreground.b == 0)
            {
                (*(spirals + *active_spiral)).foreground.b = 255;
            }
            break;

        }

    }
}


void process_key_down(int* active_channel, Spiral* spirals, int* active_spiral)
{
    if (IsKeyDown(KEY_DOWN))
    {
        switch (*active_channel)
        {
        case 0:
            (*(spirals + *active_spiral)).foreground.r -= 1;
            if ((*(spirals + *active_spiral)).foreground.r == 255)
            {
                (*(spirals + *active_spiral)).foreground.r = 0;
            }
            break;

        case 1:
            (*(spirals + *active_spiral)).foreground.g -= 1;
            if ((*(spirals + *active_spiral)).foreground.g == 255)
            {
                (*(spirals + *active_spiral)).foreground.g = 0;
            }
            break;

        case 2:
            (*(spirals + *active_spiral)).foreground.b -= 1;
            if ((*(spirals + *active_spiral)).foreground.b == 255)
            {
                (*(spirals + *active_spiral)).foreground.b = 0;
            }
            break;

        }

    }
}


void process_input(Spiral* spirals, int* active_spiral, int*active_channel)
{
    process_key_right(active_channel, active_spiral);

    process_key_left(active_channel, active_spiral);

    process_key_up(active_channel, spirals, active_spiral);

    process_key_down(active_channel, spirals, active_spiral);
}



void draw_spiral_name(Spiral* spirals, int active_spiral)
{
    //Color spiral_color = (*(spirals + active_spiral)).foreground;
    Color spiral_color = YELLOW;
    switch (active_spiral) {
    case 0:
        DrawText("Spiral Left 3", 10, 30, 12, spiral_color);
        break;
    case 1:
        DrawText("Spiral Right 3", 10, 30, 12, spiral_color);
        break;
    case 2:
        DrawText("Spiral Left 2", 10, 30, 12, spiral_color);
        break;
    case 3:
        DrawText("Spiral Right 2", 10, 30, 12, spiral_color);
        break;
    case 4:
        DrawText("Spiral Left 1", 10, 30, 12, spiral_color);
        break;
    case 5:
        DrawText("Spiral Right 1", 10, 30, 12, spiral_color);
        break;
    }
}


void draw_spiral_color(Spiral* spirals, int* active_spiral, int* active_channel)
{
    int color = 0;
    Color text_color = BLACK;
    switch (*active_channel) {
    case 0:
        DrawText("Red Channel", 10, 45, 12, YELLOW);
        color = (*(spirals + *active_spiral)).foreground.r;
        text_color = RED;
        break;
    case 1:
        DrawText("Green Channel", 10, 45, 12, YELLOW);
        color = (*(spirals + *active_spiral)).foreground.g;
        text_color = GREEN;
        break;
    case 2:
        DrawText("Blue Channel", 10, 45, 12, YELLOW);
        color = (*(spirals + *active_spiral)).foreground.b;
        text_color = BLUE;
        break;
    }

    char number[4];
    sprintf(number, "%d", color);
    DrawText(number, 100, 45, 12, text_color);
}


void draw_text(Spiral* spirals, int* active_spiral, int* active_channel)
{
    draw_spiral_name(spirals, *active_spiral);

    draw_spiral_color(spirals, active_spiral, active_channel);
}


//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 600;
static const int screenHeight = 800;


//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    Spiral spirals[6];

    Color color0 = { 0x22, 0x00, 0x00, 0xff };
    new_spiral(spirals + 0, color0, 0.92, 0.9);

    Color color1 = { 0x00, 0x22, 0x11, 0xff };
    new_spiral(spirals + 1, color1, 0.08, 0.9);

    Color color2 = { 0x66, 0x00, 0x00, 0xff };
    new_spiral(spirals + 2, color2, 0.95, 0.93);

    Color color3 = { 0x00, 0x33, 0x22, 0xff };
    new_spiral(spirals +3, color3, 0.05, 0.93);

    Color color4 = { 0xff, 0x00, 0x00, 0xff };
    new_spiral(spirals + 4, color4, 1.0, 1.0);

    Color color5 = { 0x00, 0xff, 0xcc, 0xff };
    new_spiral(spirals + 5, color5, 0.0, 1.0);

    SpiralCamera camera;
    new_camera(&camera, 10.0, 10.0, 1.0, 2.1, -5.0);

    int active_spiral = 0;
    int active_channel = 0;

    // Initialization
    //---------------------------------------------------------
    SetConfigFlags(
        FLAG_WINDOW_RESIZABLE |
        FLAG_VSYNC_HINT |
        FLAG_MSAA_4X_HINT
    );
    InitWindow(screenWidth, screenHeight, "fir-tree");

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    //SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update logic
        //---------------------------------------------------------
        process_input(spirals, &active_spiral, &active_channel);

        resize(&camera, GetScreenWidth(), GetScreenHeight());

        float frame_time = GetFrameTime();

        for (int i = 0; i < 6; i++)
        {
            compute_segment(spirals + i, frame_time);
        }


        // Render frame
        //---------------------------------------------------------
        BeginDrawing();

        ClearBackground(BLACK);

        for(int i = 0; i<6; i++)
        {
           render(spirals + i, &camera);
        }
        

        // Render UI
        //---------------------------------------------------------
        DrawFPS(10, 10);

        draw_text(spirals, &active_spiral, &active_channel);

        EndDrawing();
    }
#endif

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    // Free resources
    for (int i = 0; i < 6; i++)
    {
        free((*(spirals+i)).segment);
    }
    
    return 0;
}
