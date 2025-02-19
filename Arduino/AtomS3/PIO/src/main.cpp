/**
 * @file main.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-01-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */

 #include <Arduino.h>
 #include <vector>
 
 #define LGFX_AUTODETECT
 #include <LovyanGFX.hpp>
 static LGFX Lcd;
 static LGFX_Sprite Screen(&Lcd);
 #include <M5AtomS3.h>
 #include "AtomJoyStick.h"

 /* Game config */
 #define SNAKE_BODY_WIDTH        16
 #define GAME_UPDATE_INTERVAL    10
 #define COLOR_SNAKE             0xff8153U
 #define COLOR_SNAKE_SHADOW      0Xb55f3cU
 #define COLOR_FOOD              0x88d35eU
 #define COLOR_FOOD_SHADOW       0x5d8c3dU
 #define COLOR_EXPLODE           0x88d35eU
 #define COLOR_EXPLODE_SHADOW    0x5d8c3dU
 #define COLOR_BG_GRID           0x61497eU
 #define COLOR_BG_FRAME          0x61497eU
 #define COLOR_BG_DIALOG         0x61497eU
 #define COLOR_BG_DIALOG_SHADOW  0x443454U

 AtomJoyStick joystick;

 struct Coordinate_t {
     int x;
     int y;
 };
 
 enum MoveDirection_t {
     MOVE_UP,
     MOVE_DOWN,
     MOVE_LEFT,
     MOVE_RIGHT
 };
 
 std::vector<Coordinate_t> Snake_BodyList;
 static MoveDirection_t Snake_MoveDirection = MOVE_RIGHT;
 static MoveDirection_t Snake_MoveDirection_Old = MOVE_RIGHT;
 static Coordinate_t Food_Coor = {0, 0};
 static uint8_t Food_Size = 0;
 static uint32_t Food_UpdateTime_Old = 0;
 static Coordinate_t Explode_Coor = {0, 0};
 static uint8_t Explode_Size = 0;
 static bool Explode_Exploding = false;
 static uint32_t Explode_UpdateTime_Old = 0;
 static uint32_t BackGround_UpdateTime_Old = 0;
 static uint32_t Game_UpdateTime_LastFrame = 0;
 static unsigned int Game_Score = 0;
 
 void Game_Reset();
 void Game_Over();
 void Game_Input_Update();
 void Game_Input_Update_Callback(MoveDirection_t &MoveDirection);
 void Game_Over_Callback();
 void Game_Snake_Move();
 void Game_Snake_Grow();
 void Game_Food_Update();
 void Game_FoodExplode_Update();
 void Game_Render_Snake();
 void Game_Render_BackGround();
 void Game_Render_Food();
 void Game_Render_FoodExplode();
 void Game_Check_EatFood();
 void Game_Check_EatMyself();
 
 // ------------------------------------------------------------------ //
 
 void setup()
 {
     M5.begin(false, false, true, false);
     M5.IMU.begin();
     USBSerial.begin(115200);
 
     Lcd.init();
     //Lcd.setRotation(2);
     Screen.createSprite(Lcd.width(), Lcd.height());
    
     while (!joystick.begin(&Wire, ATOM_JOYSTICK_ADDR, 38, 39, 400000U)) {
        USBSerial.println("Couldn't find Atom JoyStick");
        delay(2000);
    }

     /* Game init */
     Game_Reset();
 }
 
 
 void loop()
 {
     /* Frame update */
     if (Game_UpdateTime_LastFrame == 0)
         Game_UpdateTime_LastFrame = millis();
     else if ((millis() - Game_UpdateTime_LastFrame) > GAME_UPDATE_INTERVAL)
     {
         Game_UpdateTime_LastFrame = millis();
 
         Game_Snake_Move();
         Game_Check_EatFood();
         Game_Check_EatMyself();
 
         Screen.clear();
         Game_Render_BackGround();
         Game_Render_Food();
         Game_Render_Snake();
         Game_Render_FoodExplode();
         Screen.pushSprite(0, 0);
     }
 
     Game_Input_Update();
 }
 
 
 void Game_Input_Update_Callback(MoveDirection_t &MoveDirection)
 {
     /* Change moving direction, e.g. MoveDirection = MOVE_RIGHT */
     /*
     float ax, ay, az = 0;
     M5.IMU.getAccel(&ax, &ay, &az);
     if (ay > 0.3)
         MoveDirection = MOVE_UP;
     else if (ay < -0.3)
         MoveDirection = MOVE_DOWN;
     else if (ax < -0.3)
         MoveDirection = MOVE_LEFT;
     else if (ax > 0.3)
         MoveDirection = MOVE_RIGHT;
    */
    uint16_t joy1_x, joy1_y;

    joy1_x = joystick.getJoy1ADCValueX(_12bit);
    joy1_y = joystick.getJoy1ADCValueY(_12bit);
    //USBSerial.printf("x: %d\n", joy1_x);
    if (joy1_y > 2500)
        MoveDirection = MOVE_DOWN;
    else if (joy1_y < 1500)
        MoveDirection = MOVE_UP;
    else if (joy1_x < 1500)
        MoveDirection = MOVE_LEFT;
    else if (joy1_x > 2500)
        MoveDirection = MOVE_RIGHT;

    //USBSerial.print(MoveDirection);
 }
 
 
 void Game_Over_Callback()
 {
     /* Call return to reset game */
     pinMode(41, INPUT);
     while (1)
     {
         if (!digitalRead(41))
         {
             delay(20);
             if (!digitalRead(41))
             {
                 while (!digitalRead(41));
                 return;
             }
         }
     }
 }
 
 // ------------------------------------------------------------------ //
 
 void Game_Reset()
 {
     Snake_BodyList.clear();
     Snake_BodyList.push_back({(int)(SNAKE_BODY_WIDTH + SNAKE_BODY_WIDTH / 2), (int)(SNAKE_BODY_WIDTH + SNAKE_BODY_WIDTH / 2)});
     Snake_MoveDirection = MOVE_RIGHT;
     Snake_MoveDirection_Old = MOVE_RIGHT;
     Game_Score = 0;
     Explode_Exploding = false;
 
     Game_Food_Update();
 }
 
 
 void Game_Over()
 {
     /* Render dialog window */
     for (int w = 0; w < ((Screen.width() * 5) / 6); w += 2)
     {
         Screen.fillRoundRect((Screen.width() / 12 + 3) + ((Screen.width() * 5) / 12) - (w / 2), Screen.height() / 10 + 3, w, (Screen.height() * 7) / 10, 8, COLOR_BG_DIALOG_SHADOW);
         Screen.fillRoundRect((Screen.width() / 12) + ((Screen.width() * 5) / 12) - (w / 2), Screen.height() / 10, w, (Screen.height() * 7) / 10, 8, COLOR_BG_DIALOG);
         Screen.pushSprite(0, 0);
     }
     delay(50);
 
     char TextBuff[10];
     int32_t FontHeight = 0;
     Screen.setFont(&fonts::Font8x8C64);
     Screen.setTextDatum(top_center);
     Screen.setTextColor(TFT_WHITE, COLOR_BG_GRID);
 
     Screen.setTextSize(Screen.width() / 128);
     snprintf(TextBuff, sizeof(TextBuff), "GAME OVER");
     Screen.drawCenterString(TextBuff, Screen.width() / 2 - 3, (Screen.height() / 2));
 
     Screen.setTextSize(Screen.width() / 42);
     FontHeight = Screen.fontHeight();
     snprintf(TextBuff, sizeof(TextBuff), "%d", Game_Score);
     for (float s = 1; s < (Screen.width() / 42); s += 0.1)
     {
         Screen.setTextSize(s);
         Screen.drawCenterString(TextBuff, Screen.width() / 2 - 3, (Screen.height() / 2) - (FontHeight / 2 * 3));
         delay(2);
         Screen.pushSprite(0, 0);
     }
 
     Screen.pushSprite(0, 0);
     Game_Over_Callback();
 
     /* Close dialog window */
     for (int w = ((Screen.width() * 5) / 6); w > 0; w -= 2)
     {
         Screen.clear();
         Game_Render_BackGround();
         Screen.fillRoundRect((Screen.width() / 12 + 3) + ((Screen.width() * 5) / 12) - (w / 2), Screen.height() / 10 + 3, w, (Screen.height() * 7) / 10, 8, COLOR_BG_DIALOG_SHADOW);
         Screen.fillRoundRect((Screen.width() / 12) + ((Screen.width() * 5) / 12) - (w / 2), Screen.height() / 10, w, (Screen.height() * 7) / 10, 8, COLOR_BG_DIALOG);
         Screen.pushSprite(0, 0);
     }
     Game_Reset();
 }
 
 
 void Game_Input_Update()
 {
     MoveDirection_t Snake_MoveDirection_New = Snake_MoveDirection;
     Game_Input_Update_Callback(Snake_MoveDirection_New);
     
     /* No backward */
     switch (Snake_MoveDirection_New)
     {
         case MOVE_UP:
             if (Snake_MoveDirection_Old == MOVE_DOWN)
                 return;
             break;
         case MOVE_DOWN:
             if (Snake_MoveDirection_Old == MOVE_UP)
                 return;
             break;
         case MOVE_LEFT:
             if (Snake_MoveDirection_Old == MOVE_RIGHT)
                 return;
             break;
         case MOVE_RIGHT:
             if (Snake_MoveDirection_Old == MOVE_LEFT)
                 return;
             break;
         default:
             return;
     }
     Snake_MoveDirection = Snake_MoveDirection_New;
 }
 
 
 void Game_Snake_Move()
 {
     Coordinate_t Coor_New = *Snake_BodyList.begin();
 
     /* Turn when hit the right point */
     if (((Coor_New.x + (SNAKE_BODY_WIDTH / 2)) % SNAKE_BODY_WIDTH) == 0)
     {
         if (((Coor_New.y + (SNAKE_BODY_WIDTH / 2)) % SNAKE_BODY_WIDTH) == 0)
             Snake_MoveDirection_Old = Snake_MoveDirection;
     }
 
     switch (Snake_MoveDirection_Old)
     {
         case MOVE_UP:
             Coor_New.y -= 1;
             break;
         case MOVE_DOWN:
             Coor_New.y += 1;
             break;
         case MOVE_LEFT:
             Coor_New.x -= 1;
             break;
         case MOVE_RIGHT:
             Coor_New.x += 1;
             break;
         default:
             break;
     }
 
     /* If hit the wall */
     if (Coor_New.x < 0)
         Coor_New.x = Screen.width();
     if (Coor_New.y < 0)
         Coor_New.y = Screen.height();
     if (Coor_New.x > Screen.width())
         Coor_New.x = 0;
     if (Coor_New.y > Screen.height())
         Coor_New.y = 0;
 
     Snake_BodyList.insert(Snake_BodyList.begin(), Coor_New);
     Snake_BodyList.pop_back();
 }
 
 
 void Game_Snake_Grow()
 {
     for (int i = 0; i < SNAKE_BODY_WIDTH; i++)
         Snake_BodyList.push_back(Snake_BodyList.back());
 }
 
 
 void Game_Render_Snake()
 {
     for (auto i : Snake_BodyList)
         Screen.fillRoundRect(i.x - (SNAKE_BODY_WIDTH / 2) + 2, i.y - (SNAKE_BODY_WIDTH / 2) + 2, SNAKE_BODY_WIDTH, SNAKE_BODY_WIDTH, 1, COLOR_SNAKE_SHADOW);
     for (auto i : Snake_BodyList)
         Screen.fillRoundRect(i.x - (SNAKE_BODY_WIDTH / 2), i.y - (SNAKE_BODY_WIDTH / 2), SNAKE_BODY_WIDTH, SNAKE_BODY_WIDTH, 1, COLOR_SNAKE);
 }
 
 
 void Game_Food_Update()
 {
     /* Get a random Y */
     while (1)
     {
         Food_Coor.x = random(0 + (SNAKE_BODY_WIDTH / 2), Screen.width() - (SNAKE_BODY_WIDTH / 2));
         /* Check if fit grid */
         if (((Food_Coor.x + (SNAKE_BODY_WIDTH / 2)) % SNAKE_BODY_WIDTH) != 0)
             continue;
         /* Check if hit snake */
         for (auto i : Snake_BodyList) 
         {
             if (Food_Coor.x == i.x)
                 continue;
         }
         break;
     }
     /* Get a random Y */
     while (1)
     {
         Food_Coor.y = random(0 + (SNAKE_BODY_WIDTH / 2), Screen.height() - (SNAKE_BODY_WIDTH / 2));
         /* Check if fit grid */
         if (((Food_Coor.y + (SNAKE_BODY_WIDTH / 2)) % SNAKE_BODY_WIDTH) != 0)
             continue;
         /* Check if hit snake */
         for (auto i : Snake_BodyList) 
         {
             if (Food_Coor.y == i.y)
                 continue;
         }
         break;
     }
     /* Resize food */
     Food_Size = 0;
 }
 
 
 void Game_FoodExplode_Update()
 {
     Explode_Exploding = true;
     Explode_Coor.x = Snake_BodyList.begin()->x;
     Explode_Coor.y = Snake_BodyList.begin()->y;
     Explode_Size = 2;
 }
 
 
 void Game_Render_Food()
 {
     Screen.fillRoundRect(Food_Coor.x - (Food_Size / 2) + 2, Food_Coor.y - (Food_Size / 2) + 2, Food_Size, Food_Size, 1, COLOR_FOOD_SHADOW);
     Screen.fillRoundRect(Food_Coor.x - (Food_Size / 2), Food_Coor.y - (Food_Size / 2), Food_Size, Food_Size, 1, COLOR_FOOD);
 
     /* Food grow up */
     if (Food_UpdateTime_Old == 0)
         Food_UpdateTime_Old = millis();
     else
     {
         if ((millis() - Food_UpdateTime_Old) > 10)
         {
             Food_UpdateTime_Old = millis();
             if (Food_Size < SNAKE_BODY_WIDTH)
                 Food_Size++;
         }
     }
 }
 
 
 void Game_Render_FoodExplode()
 {
     if (Explode_Exploding)
     {
         /* Render octagon explode */
         int32_t LittleFood_Size = SNAKE_BODY_WIDTH / 2;
         Screen.fillRoundRect(2 + Explode_Coor.x + Explode_Size, 2 + Explode_Coor.y, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x - Explode_Size, 2 + Explode_Coor.y, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x, 2 + Explode_Coor.y + Explode_Size, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x, 2 + Explode_Coor.y - Explode_Size, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x + (Explode_Size * 7 / 10), 2 + Explode_Coor.y - (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x + (Explode_Size * 7 / 10), 2 + Explode_Coor.y + (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x - (Explode_Size * 7 / 10), 2 + Explode_Coor.y + (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(2 + Explode_Coor.x - (Explode_Size * 7 / 10), 2 + Explode_Coor.y - (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE_SHADOW);
         Screen.fillRoundRect(Explode_Coor.x + Explode_Size, Explode_Coor.y, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x - Explode_Size, Explode_Coor.y, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x, Explode_Coor.y + Explode_Size, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x, Explode_Coor.y - Explode_Size, LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x + (Explode_Size * 7 / 10), Explode_Coor.y - (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x + (Explode_Size * 7 / 10), Explode_Coor.y + (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x - (Explode_Size * 7 / 10), Explode_Coor.y + (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
         Screen.fillRoundRect(Explode_Coor.x - (Explode_Size * 7 / 10), Explode_Coor.y - (Explode_Size * 7 / 10), LittleFood_Size, LittleFood_Size, 1, COLOR_EXPLODE);
 
         if (Explode_UpdateTime_Old == 0)
             Explode_UpdateTime_Old = millis();
         else if ((millis() - Explode_UpdateTime_Old) > 15)
             {
                 Explode_UpdateTime_Old = millis();
 
                 if (Explode_Size < (SNAKE_BODY_WIDTH * 2))
                     Explode_Size++;
                 else
                     Explode_Exploding = false;
             }
     }
 }
 
 
 void Game_Check_EatFood()
 {
     for (auto i : Snake_BodyList)
     {
         if ((Food_Coor.x == i.x) && (Food_Coor.y == i.y))
         {
             Game_Score++;
             Game_Snake_Grow();
             Game_Food_Update();
             Game_FoodExplode_Update();
         }
     }
 }
 
 
 void Game_Check_EatMyself()
 {
     /* ***Check bug, not yet figured out */
     if (Snake_BodyList.size() < (3 * (SNAKE_BODY_WIDTH / 2)))
         return;
 
     Coordinate_t Coor_Head = *Snake_BodyList.begin();
     for (auto i = Snake_BodyList.begin() + 1; i <= Snake_BodyList.end(); i++)
     {
         if ((Coor_Head.x == i->x) && (Coor_Head.y == i->y))
             Game_Over();
     }
 }
 
 
 void Game_Render_BackGround()
 {   
     /* Draw score */
     char TextBuff[10];
     //Screen.setFont(&fonts::Font8x8C64);
     Screen.setTextFont(7);
     Screen.setTextDatum(top_center);
     //Screen.setTextSize(Screen.width() / 20);
     Screen.setTextSize(2);
     snprintf(TextBuff, sizeof(TextBuff), "%d", Game_Score);
     Screen.setTextColor(COLOR_BG_DIALOG_SHADOW, TFT_BLACK);
     Screen.drawCenterString(TextBuff, Screen.width() / 2 - 1, (Screen.height() / 2) - Screen.getTextSizeY() * 12);
     Screen.setTextColor(COLOR_BG_GRID, TFT_BLACK);
     Screen.drawCenterString(TextBuff, Screen.width() / 2 - 5, (Screen.height() / 2) - Screen.getTextSizeY() * 12);
 
     /* Draw grid */
     for (int x = -(SNAKE_BODY_WIDTH / 2) - 1; x < Screen.width(); x += SNAKE_BODY_WIDTH)
         for (int y = -(SNAKE_BODY_WIDTH / 2); y < Screen.height(); y += SNAKE_BODY_WIDTH)
             Screen.drawPixel(x, y, COLOR_BG_GRID);
 }