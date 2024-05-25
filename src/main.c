#include <raylib.h>

int main()
{
    InitWindow(1280,720,"TITLEKEKW");
    while(!WindowShouldClose())
    {
        BeginDrawing();
        DrawCircle(GetScreenWidth()/2,GetScreenHeight()/2,100,RED);
        EndDrawing();
    }   
}