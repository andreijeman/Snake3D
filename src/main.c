#include <windows.h>
#include <gl/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_easy_font.h"

#include "list.c"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

//----------------------------------------------------------------------------------------------------------------------------------

GLuint windowHeight;
GLuint windowWidth;

struct Camera{
    GLfloat x,y,z;
    GLfloat xRot,zRot;
} camera = {0,0,5, 0,0};

void apply_camera()
{
    glRotatef(-camera.xRot, 1,0,0);
    glRotatef(-camera.zRot, 0,0,1);
    glTranslatef(-camera.x, -camera.y, -camera.z);
}

void rotate_camera(GLfloat xAngle, GLfloat zAngle)
{
    camera.xRot += xAngle;
    if(camera.xRot < 0) camera.xRot = 0;
    if(camera.xRot > 180) camera.xRot = 180;

    camera.zRot += zAngle;
    if(camera.zRot < 0) camera.zRot += 360;
    if(camera.zRot > 360) camera.zRot = 0;
}

void move_camera_with_mouse(GLfloat xSpeed, GLfloat ySpeed)
{
    POINT cursor;
    POINT origin = {800, 400};
    GetCursorPos(&cursor);
    rotate_camera((origin.y - cursor.y)*ySpeed, (origin.x-cursor.x)*xSpeed);
    SetCursorPos(origin.x, origin.y);
}

void set_camera_pos_x_y(GLfloat zAngle, GLfloat speed)
{
    if(speed != 0)
    {
        camera.x += sin(zAngle)*speed;
        camera.y += cos(zAngle)*speed;
    }
}

void move_camera_with_button(float speed)
{
    float zAngle = -camera.zRot / 180 * M_PI;
    if(GetKeyState('W') < 0) set_camera_pos_x_y(zAngle, speed);
    if(GetKeyState('S') < 0) set_camera_pos_x_y(zAngle, -speed);
    if(GetKeyState('A') < 0) set_camera_pos_x_y(zAngle-M_PI/2, speed);
    if(GetKeyState('D') < 0) set_camera_pos_x_y(zAngle+M_PI/2, speed);

    if(GetKeyState(VK_SPACE) < 0) camera.z += speed;
    if(GetKeyState(VK_LSHIFT) < 0) camera.z -= speed;
}

void resize_window(int x, int y)
{
    glViewport(0,0,x,y);
    float k = (float)x/y;
    float size = 0.1;
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glFrustum(-k*size, k*size, -size,size, size*2, 100);
}

void print_string(float x, float y, char *text, float r, float g, float b)
{
  static char buffer[99999]; // ~500 chars
  int num_quads;

  num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));

  glColor3f(r,g,b);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 16, buffer);
  glDrawArrays(GL_QUADS, 0, num_quads*4);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void load_texture(char *file_name, int *target)
{
    int width, height, cnt;
    unsigned char *data = stbi_load(file_name, &width, &height, &cnt, 0);

    glGenTextures(1, target);
    glBindTexture(GL_TEXTURE_2D, *target);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA , width, height, 0, cnt == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(data);
}

#define SIZE_MAP 32

int tex_moss;

void draw_map()
{
    glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_moss);
        glBegin(GL_QUADS);
            glColor3f(0.6,1,0.6);
            glTexCoord2f(0, 0); glVertex2f(0,0);
            glTexCoord2f(SIZE_MAP, 0); glVertex2f(SIZE_MAP,0);
            glTexCoord2f(SIZE_MAP, SIZE_MAP); glVertex2f(SIZE_MAP,SIZE_MAP);
            glTexCoord2f(0, SIZE_MAP); glVertex2f(0,SIZE_MAP);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void draw_sphere(float x, float y, float z, float r)
{
    float step = M_PI/6;
    glPushMatrix();
    glTranslatef(x,y,z);
    glBegin(GL_TRIANGLE_STRIP);
        for(float i = 0; i <= M_PI-step; i += step)
        {
            for(float j = 0; j <= M_PI*2; j += step)
            {
                glVertex3f(sin(j)*r*sin(i), cos(j)*r*sin(i), z+r*cos(i));
                glVertex3f(sin(j)*r*sin(i+step), cos(j)*r*sin(i+step), z+r*cos(i+step));
            }
        }
     glEnd();
     glPopMatrix();
}

typedef struct Data
{
    float x,y,z;
    float xAngle;
    float r;
} Data;

void draw_snake(List *list)
{
    if(list)
    {
        if(list->head)
        {
            Data *data = (Data *)list->head->data;
            glColor3f(1,0,0);
            draw_sphere(data->x, data->y, data->z/2, data->r*1.3); //cap
            glColor3f(0,0,0);
            draw_sphere(data->x+data->r*cos((data->xAngle+90)/180*M_PI), data->y+data->r*sin((data->xAngle+90)/180*M_PI), data->z/1.5, data->r/2); //ochi
            draw_sphere(data->x-data->r*cos((data->xAngle+90)/180*M_PI), data->y-data->r*sin((data->xAngle+90)/180*M_PI), data->z/1.5, data->r/2); //ochi

            glBegin(GL_TRIANGLE_STRIP);
            for(Node *current_node = list->head; current_node->next; current_node = current_node->next)
            {
                Data *data1 = (Data *)current_node->data;
                Data *data2 = (Data *)current_node->next->data;

                    glColor3ub(255, 255/data1->r, data1->r*10);
                    for(float angle = 0; angle < 2*M_PI; angle += M_PI/6)
                    {
                        glVertex3f(data1->x+data1->r*cos((data1->xAngle+90)/180*M_PI)*cos(angle),
                                   data1->y+data1->r*sin((data1->xAngle+90)/180*M_PI)*cos(angle),
                                   data1->z+data1->r*sin(angle));

                        glVertex3f(data2->x+data2->r*cos((data2->xAngle+90)/180*M_PI)*cos(angle),
                                   data2->y+data2->r*sin((data2->xAngle+90)/180*M_PI)*cos(angle),
                                   data2->z+data2->r*sin(angle));
                    }
            }
            glEnd();

            data = (Data *)list->tail->data;
            draw_sphere(data->x, data->y, data->z/2, data->r); //poponeata
        }
    }
}


void move_snake(List *list, float speed, float speedRot, int ctrl_up, int ctrl_left, int ctrl_right)
{
    if(list)
    {
        if(list->head)
        {
            if(GetKeyState(ctrl_up) < 0)
            {
                Data *data;
                for(Node *current_node = list->tail; current_node->back; current_node = current_node->back) //interschimbab toate segmentele
                {
                    Data *current_data = (Data *)current_node->data;
                    Data *back_data = (Data *)current_node->back->data;
                    current_data->xAngle = back_data->xAngle;
                }

                data = (Data *)list->head->data;

                if(GetKeyState(ctrl_left) < 0)
                {
                    data->xAngle += speedRot;
                    if(data->xAngle > 360) data->xAngle = 0;
                }
                if(GetKeyState(ctrl_right) < 0)
                {
                    data->xAngle -= speedRot;
                    if(data->xAngle < 0) data->xAngle = 360;
                }

                float xAngleRad = data->xAngle / 180 * M_PI;
                //deplasam camera dupa sarpe
                if(data->x > 0 && data->x < SIZE_MAP) camera.x += cos(xAngleRad)*speed;
                if(data->y > 0 && data->y < SIZE_MAP) camera.y += sin(xAngleRad)*speed;

                for(Node *current_node = list->head; current_node; current_node = current_node->next) //calc x si y
                {
                    data = (Data *)current_node->data;
                    xAngleRad = data->xAngle / 180 * M_PI;
                    data->x += cos(xAngleRad)*speed;
                    data->y += sin(xAngleRad)*speed;

                    if(data->x < 0) data->x = 0;
                    else if(data->x > SIZE_MAP) data->x = SIZE_MAP;
                    if(data->y < 0) data->y = 0;
                    else if(data->y > SIZE_MAP) data->y = SIZE_MAP;

                }

            }
        }
    }
}
void init_snake(List *list, float len1, float len2, float step, float size)
{
    if(list)
    {
        list->size = len2-len1;
        for(float i = len1; i < len2; i++)
        {
            Data *data = malloc(sizeof(Data));
            data->x = step*i;
            data->y = 0;
            data->z = i*step*size;
            data->r = i*step*size;
            data->xAngle = 0;
            push_front(list, data);
        }
    }
}

void grow_snake(List *list, float len, float step, float size)
{
    if(list)
    {
        list->size += len;

        for(Node *current_node = list->head; current_node; current_node = current_node->next)
        {
            Data *data = (Data *)current_node->data;
            data->r = (data->r/size+len*step)*size;
            data->z = data->r;
        }

        if(list->head)
        {
            Data *data_tail = (Data *)list->tail->data;
            float x_step = step*cos(data_tail->xAngle/180*M_PI);
            float y_step = step*sin(data_tail->xAngle/180*M_PI);

            for(int i = 1; i < len; i++)
            {
                Data *data = malloc(sizeof(Data));
                data->x = data_tail->x - x_step*i;
                data->y = data_tail->y - y_step*i;

                data->z = data_tail->z - i*step*size;
                data->r = data_tail->r - i*step*size;
                data->xAngle = data_tail->xAngle;
                push_back(list, data);
            }
        }
    }
}


void cut_tail_snake(List *list, int len)
{
    if(list)
    {
        Node *node = list->tail;
        for(int i = 0; i < len; i++)
        {
            free(node->data);
            Node *temp = node->back;
            free(node);
            node = temp;
        }
        node->next = NULL;
        list->tail = node;
    }
}

typedef struct Food
{
    float x,y,r;
} Food;

int rand_num(int min, int max) {
    max -= min;
    ++max;
    return rand() % max + min;
}

void init_food(Food *foods, int amount)
{
    for(int i = 0; i < amount; i++)
    {
        foods[i].x = rand_num(0, SIZE_MAP);
        foods[i].y = rand_num(0, SIZE_MAP);
        foods[i].r = 0.2;
    }
}

void eat_food(List *list, Food *food, int amount_food)
{
    Data *data_head = (Data *)list->head->data;
    for(int i = 0; i < amount_food; i++)
    {
        if(data_head->x > food[i].x - data_head->r && data_head->x < food[i].x + data_head->r &&
           data_head->y > food[i].y - data_head->r && data_head->y < food[i].y + data_head->r)
            {
                food[i].x = rand_num(0, SIZE_MAP);
                food[i].y = rand_num(0, SIZE_MAP);
                food[i].r = data_head->r*1.5;

                grow_snake(list, 30, 0.1, 0.005);
                cut_tail_snake(list, 15);
            }
    }
}

void draw_food(Food *food, int amount)
{
    for(int i = 0; i < amount; i++)
    {
        draw_sphere(food[i].x, food[i].y, food[i].r/3, food[i].r);
    }
}

void draw_num(int num, float size, float x, float y, float z)
{
    glTranslatef(x, y, z);
    glScalef(size,-size,size);

    int n = num;
    int len = 0;
    while(n)
    {
        n /= 10;
        len++;
    }

    char str_num[len];
    str_num[len] = '\0';
    n = num;

    for(int i = len-1; i >= 0; i--)
    {
        str_num[i] = n%10 + '0';
        n /= 10;
    }

    print_string(0,0, str_num, 1,1,1);
}

//-----------------------------------------------------------------------------------------------------------------------------------


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;
    float theta = 0.0f;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "OpenGL Sample",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          800,
                          600,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);

//----------------------------------------------------------------------------------
    srand(time(NULL));

    load_texture("moss_block.png", &tex_moss);

    RECT rct;
    GetClientRect(hwnd, &rct);
    resize_window(rct.right, rct.bottom);


    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    List *snake = create_list();
    init_snake(snake, 200, 250, 0.1, 0.005);

    Food foods[5];
    init_food(foods, 5);


//----------------------------------------------------------------------------------
    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */

            glClearColor(0.3f, 0.7f, 1, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glPushMatrix();

                if(GetForegroundWindow() == hwnd)
                {
                    move_camera_with_mouse(0.2, 0.2);
                    move_camera_with_button(0.1);
                    //printf("(X:%.1f, Y:%.1f, Z:%.1f, xR:%.1f, zR:%.1f)\n", camera.x, camera.y, camera.z, camera.xRot, camera.zRot);
                }

                move_snake(snake, 0.1, 5, VK_UP, VK_LEFT, VK_RIGHT);
                //Data *data = snake->head->data;
                //printf("(X:%.1f, Y:%.1f, Z:%.1f, xA:%.1f)\n", data->x, data->y, data->z, data->xAngle);
                eat_food(snake, foods, 10);

                apply_camera(camera);

                draw_map();
                draw_snake(snake);
                draw_food(foods, 5);
                draw_num(snake->size, 0.2, 0.7, 2, 0.1);

            glPopMatrix();


            SwapBuffers(hDC);

            theta += 0.001;
            Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        //---------------------------------------------------------------------------------
        case WM_SIZE:
            windowWidth = LOWORD(lParam);
            windowHeight = HIWORD(lParam);
            resize_window(windowWidth, windowHeight);
        break;

        case WM_SETCURSOR:
            ShowCursor(FALSE);
        break;
        //-----------------------------------------------------------------------------------
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}
