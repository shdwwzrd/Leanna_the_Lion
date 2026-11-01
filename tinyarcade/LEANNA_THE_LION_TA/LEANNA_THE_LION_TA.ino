/*
  Leanna The Lion: http://tinyarcade.neoretro.games#leanna_the_lion
  Tiny Arcade version 1.0
  MADE by Neo Retro Games
  2017 - SHDWWZRD KURONOWACHAN CRIS
  License: MIT : https://opensource.org/licenses/MIT
 
	
*/

#include <TinyScreen.h>
#include "TinyArcade.h"
#include "neoretrolib.h"
#include "images.h"
#include "SoundPlayer.hpp"
#include "sounds.h"
#include "SdFat.h"



TinyScreen tiny_screen = TinyScreen(TinyScreenPlus);

bool SAVE_HIGH_SCORE=true;

//define menu states (on main menu)
#define STATE_MENU_INTRO         0 //show group logo
#define STATE_SAVE_HIGH_SCORE    1
#define STATE_TITLE_SCREEN       2 //a button to play, up/down - sound on/off,
#define STATE_STORY_SCREEN       3 //story
#define STATE_GAME_PLAYING       4 //main game playing screen
#define STATE_GAME_OVER          5 //game over text on screen, show score and level reached returns to title screen after a button clicked
#define STATE_CREDITS_SCREEN     6
#define STATE_HISCORE_SCREEN     7

#define VERSION "V1.0"


/////////////////////////////////////////////////////////////////////////////
//GLOBAL VARIABLES
/////////////////////////////////////////////////////////////////////////////

SdFat sd_fat;
char* file_buffer;

int8_t gameState;//state the game is in

//player variables and images
struct player_structure {
  public:
    int8_t x = 0;
    int8_t y = 38;
    int8_t row = 38;
    uint8_t frame = 0;
    uint32_t SCORE = 0;
    bool sliding = false;
    int8_t slide_timer = 0;
    int8_t jump = 0;
    int16_t multiplier_timer = 0;
};

player_structure player;

#define ELDER_ENTITY 1
#define CHILD_ENTITY 2
#define ADULT_ENTITY 3
#define MONKEY_ENTITY 4
#define ELEPHANT_ENTITY 5
#define TRANQ_ZOO_KEEPER_ENTITY 6
#define TRANQ_TRUCK_ENTITY 7
#define CAGE_ENTITY 8
#define WATER_PUDDLE_ENTITY 9
#define HOT_SAUCE_ENTITY 10
struct Entity {
  public:
    Point point = {.x = 0, .y = 0};
    bool      Exists = false;
    int8_t    frame = 0;
    int8_t    Type;
    int8_t    row;
    int8_t    width;
    int8_t    creation_timer;
    int8_t    darts = 0;
};
#define TotalEntities 4
Entity entities[TotalEntities];


#define TRANQ_DART 1
#define ANGLED_TRANQ_DART 2
#define MONKEY_POO 3
struct Dart {
  public:
    Point point = {.x = 0, .y = 0};
    bool      Exists = false;
    int8_t    Type;
    int8_t    row;
};
#define TotalDarts 4
Dart darts[TotalDarts];

bool video_out = false;

int frame_counter = 0;
int8_t bg_fence_x = 0;
int8_t fg_fence_x = 0;
int32_t game_timer = 0;
int16_t hotsauce_timer = 3600;

struct high_score_struct{
  uint16_t score;
} ;
high_score_struct high_score_data;

//odds per entity per 15 sec chunk
const unsigned char PROGMEM odds[] =
{
  //count_down_till, entity1, entity2, entity3, entity4, entity5
  //0-15 secs
  120, 0, 0, 0, 0, 0,//entity 0 row 0
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,//entity 1 row 1
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, ADULT_ENTITY, CHILD_ENTITY,//entity 2 row 2
  120, ELDER_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,//entity 3 row 3
  //16-30 secs
  120, 0, 0, 0, 0, 0,//entity 0 row 0
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,//entity 1 row 1
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY,//entity 2 row 2
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, CAGE_ENTITY,//entity 3 row 3
  //31-45 secs
  120, TRANQ_TRUCK_ENTITY, 0, 0, 0, 0,//entity 0 row 0
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,//entity 1 row 1
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, CAGE_ENTITY,//entity 2 row 2
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,//entity 3 row 3
  //46-60 secs
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0, 0, 0,//entity 0 row 0
  120, MONKEY_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,//entity 1 row 1
  120, WATER_PUDDLE_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 2 row 2
  120, MONKEY_ENTITY, ELDER_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY,//entity 3 row 3
  //61-75 secs
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0, 0,//entity 0 row 0
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 1 row 1
  120, MONKEY_ENTITY, CHILD_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY,//entity 2 row 2
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 3 row 3
  //76-90 secs
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, 0,//entity 0 row 0
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 1 row 1
  120, MONKEY_ENTITY, CHILD_ENTITY, ADULT_ENTITY, ELEPHANT_ENTITY, ELEPHANT_ENTITY,//entity 2 row 2
  120, WATER_PUDDLE_ENTITY, ELDER_ENTITY, CHILD_ENTITY, CHILD_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 3 row 3
  //91-105 secs
  120, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY, TRANQ_TRUCK_ENTITY,//entity 0 row 0
  120, MONKEY_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 1 row 1
  120, WATER_PUDDLE_ENTITY, ADULT_ENTITY, ADULT_ENTITY, CAGE_ENTITY, CAGE_ENTITY,//entity 2 row 2
  120, MONKEY_ENTITY, ADULT_ENTITY, CHILD_ENTITY, ELEPHANT_ENTITY, TRANQ_ZOO_KEEPER_ENTITY,//entity 3 row 3
};
#define MAX_LEVEL 6

const char hi_score_text[] PROGMEM = "HI SCORE:";
#define TRAMPLED_TEXT 1
#define KNOCKED_OUT_TEXT 2
#define CAGED_TEXT 3
const char trampled_text[] PROGMEM = "TRAMPLED";
const char knocked_out_text[] PROGMEM = "KNOCKED OUT";
const char caged_text[] PROGMEM = "CAGED";
int8_t gameover_type = 0;
int8_t gameover_text_id = 0;
const char gameover1_text[] PROGMEM = "What, no mint?";
const char gameover2_text[] PROGMEM = "All that fast\nfood was worth\nit.";
const char gameover3_text[] PROGMEM = "Bones can be\ntoothpicks, I\nsuppose.";
const char gameover4_text[] PROGMEM = "I demand to see\nyour\nmanager. I'm\nsure they'll\nbe delicious.";

const char story_text[] PROGMEM = "Leanna may be a\nlion, but she knowswhat she wants:\nShe wants fresh\nmeat from her\nfavorite restaurantThe zoo had to\nclose";



int eat_handle;
int game_over_handle;
int intro_music_handle;

bool SAVE_FAILED = false;


void setup() {
  tiny_screen.begin();
  tiny_screen.setBitDepth(TSBitDepth8);
  tiny_screen.setBrightness(7);

  tiny_screen.setFont(liberationSansNarrow_12ptFontInfo);
  tiny_screen.fontColor(TS_8b_White, TS_8b_Black);
  
  if(!Load()){
    //high_score_data.score=0;
    //Save();
  }
  arcadeInit();
  randomSeed(micros());
  SoundPlayer::Initialize(11025);
  SoundPlayer& sound_player = SoundPlayer::GetInstance();
  eat_handle = sound_player.Append(1, eat2_wave, static_cast<int>(sizeof(eat2_wave)), false);
  game_over_handle = sound_player.Append(1, game_over_wave, static_cast<int>(sizeof(game_over_wave)), false);
  intro_music_handle = sound_player.Append(1, intro_music_wave, static_cast<int>(sizeof(intro_music_wave)), false);
  
  gameState  = STATE_MENU_INTRO;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!(nextFrame())) return;
  pollButtons();
  pollJoystick();
  SoundPlayer& sound_player = SoundPlayer::GetInstance();
  int8_t p=0;
  char buf[5];
  switch (gameState) {
    case STATE_MENU_INTRO:
      memset(screen_buffer, COLOR8(0, 0, 0), BUFFER_SIZE);// Clear buffer
      DrawSprite(16, 0, neoretrologo2, 0, SPRITE);
      frame_counter++;
      if (frame_counter > 90) {
        frame_counter = 0;
        gameState = STATE_TITLE_SCREEN;  
        sound_player.Play(intro_music_handle);
      }
      break;
    case STATE_TITLE_SCREEN: 
      memset(screen_buffer, COLOR8(0, 0, 0), BUFFER_SIZE);// Clear buffer   
      if (joystickJustPressed(TAJoystickLeft)){
        gameState=STATE_CREDITS_SCREEN;
      }
      if (joystickJustPressed(TAJoystickRight)){
        gameState=STATE_STORY_SCREEN;
      }
      if(justPressed(TAButton1) || justPressed(TAButton2)){
        game_setup();
      }
      //game logo
      drawString(48,0,PSTR("LEANNA THE LION"),eightPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      //VERSION NUMBER
      drawString(96,8,PSTR(VERSION),fourPxFont,255,NULL,ALIGN_RIGHT,INPROGMEM);
      //menu
      drawString(0,60,PSTR("<-CREDITS"),fourPxFont,255,NULL,ALIGN_LEFT,INPROGMEM);
      drawString(96,60,PSTR("STORY->"),fourPxFont,255,NULL,ALIGN_RIGHT,INPROGMEM);
      /*     
      //show hi score*/
      drawString(48,24,PSTR(hi_score_text),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      itoa(high_score_data.score, buf, 10);
      drawString(48,32,buf,eightPxFont,255,NULL,ALIGN_CENTER, INRAM); 
      if(SAVE_FAILED){
        drawString(48,40,PSTR("SAVE FAILED"),fourPxFont,255,NULL,ALIGN_CENTER, INPROGMEM); 
      }
      break;
    case STATE_CREDITS_SCREEN:  
      memset(screen_buffer, COLOR8(0, 0, 0), BUFFER_SIZE);// Clear buffer
      //credits
      drawString(48,5,PSTR("CREDITS"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,15,PSTR("CODE & ART"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,20,PSTR("SHDWWZRD"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,30,PSTR("IDEA & ART"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,35,PSTR("KURONOWACHAN"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,45,PSTR("SPECIAL THANKS"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      drawString(48,50,PSTR("CRIS"),fourPxFont,255,NULL,ALIGN_CENTER,INPROGMEM);
      if (justPressed(TAButton1) || justPressed(TAButton2) || joystickJustPressed(TAJoystickRight)){
        gameState = STATE_TITLE_SCREEN; 
      }
      break;  
    case STATE_STORY_SCREEN:  
      memset(screen_buffer, COLOR8(0, 0, 0), BUFFER_SIZE);// Clear buffer
      if (justPressed(TAButton1) || justPressed(TAButton2) || joystickJustPressed(TAJoystickLeft)){
        gameState = STATE_TITLE_SCREEN; 
      }
      //draw story on the screen 
      drawString(0,0,PSTR(story_text),fourPxFont,255,NULL,ALIGN_LEFT,INPROGMEM); 
      break; 
    case STATE_GAME_PLAYING:
      memset(screen_buffer, COLOR8(0, 135, 81), BUFFER_SIZE);// Clear buffer
      game_timer++;
      if(hotsauce_timer>0){hotsauce_timer--;}
      
      bg_fence_x--;
      if (bg_fence_x < -15) {
        bg_fence_x = 0;
      }
      //drawLine(0,2,96,2,0);
      drawHLine(0, 2, 96, 0);
      drawHLine(0, 5, 96, 0);
      drawHLine(0, 8, 96, 0);
      for (int i = 0; i < 7; i++) {
        DrawSprite(i * 16 + bg_fence_x, 0, backgroundfence_plus_mask, 0, SPRITE_WITH_MASK);
      }

      tickEntities(0);

      if (player.sliding) {
        if (player.slide_timer > 0) {
          player.slide_timer--;
        }
        if (player.slide_timer == 0) {
          player.sliding = false;
        }
        player.frame = 4;
      } else {
        //if (checkJoystick(TAJoystickUp) && player.jump != 1) { //MOVE UP
        if (joystickJustPressed(TAJoystickUp) && player.jump != 1) { //MOVE UP
          player.row -= 12;
          if (player.row < 26) {
            player.row = 26;
          }
        }
        //if (checkJoystick(TAJoystickDown) && player.jump != 1) { //MOVE DOWN
        if (joystickJustPressed(TAJoystickDown) && player.jump != 1) { //MOVE DOWN
          player.row += 12;
          if (player.row > 50) {
            player.row = 50;
          }
        }
        //if (checkButton(TAButton1) && player.jump == 0) {
        if (justPressed(TAButton1) && player.jump == 0) {
          player.jump = 1;
        }
        if (player.jump == 1) { //leap forward quickly
          if (player.x < 32) {
            player.x += 2;
          }
          if (player.x == 32) {
            player.jump = 2;
          }
          player.frame = 5;
        } else {
          if (everyXFrames(5)) {
            player.frame++;
          }
          if (player.frame > 3) {
            player.frame = 0;
          }
        }
        if (player.jump == 2 && player.x != 0) {
          player.x--;
          if (player.x == 0) {
            player.jump = 0;
          }
        }

        if (player.y < player.row) {
          player.y++;
        } else if (player.y > player.row) {
          player.y--;
        }

      }

      fg_fence_x -= 2;
      if (fg_fence_x < -31) {
        fg_fence_x = 0;
      }
      drawHLine(0, 14, 96, 0);
      drawHLine(0, 15, 96, 0);
      drawHLine(0, 17, 96, 0);
      for (int i = 0; i < 4; i++) {
        DrawSprite(i * 32 + fg_fence_x, 14, foregroundfence_plus_mask, 0, SPRITE_WITH_MASK);
      }

      tickEntities(1);
      if(entities[2].Type == ELEPHANT_ENTITY){//player is behind row 2
        p=1;
        DrawSprite(player.x, player.y, lion_plus_mask, player.frame, SPRITE_WITH_MASK);
      }
      tickEntities(2);
      if(entities[3].Type == ELEPHANT_ENTITY){//player is behind row 3
        p=1;
        DrawSprite(player.x, player.y, lion_plus_mask, player.frame, SPRITE_WITH_MASK);
      }
      tickEntities(3);
      if(p==0){//player is in front of everything
        p=1;
        DrawSprite(player.x, player.y, lion_plus_mask, player.frame, SPRITE_WITH_MASK);
      }


      drawHLine(0, 59, 96, 0);
      drawHLine(0, 60, 96, 0);
      drawHLine(0, 62, 96, 0);
      for (int i = 0; i < 4; i++) {
        DrawSprite(i * 32 + fg_fence_x, 59, foregroundfence_plus_mask, 0, SPRITE_WITH_MASK);
      }

      tickDarts();


      
      
      isEntityCollision();
      isDartCollision();
      if(player.multiplier_timer>0){
        player.multiplier_timer--;
        drawString(96,0,PSTR("MULTIPLIER"),eightPxFont,255,NULL,ALIGN_RIGHT, INPROGMEM); 
      }
      
      itoa(player.SCORE, buf, 10);
      drawString(0,0,buf,eightPxFont,255,NULL,ALIGN_LEFT, INRAM); 
      break;
    case STATE_GAME_OVER:
      memset(screen_buffer, COLOR8(0, 0, 0), BUFFER_SIZE);// Clear buffer
      if(gameover_type==TRAMPLED_TEXT){
        drawString(48,16,PSTR(trampled_text),eightPxFont,255,NULL,ALIGN_CENTER, INPROGMEM); 
      }else if(gameover_type==KNOCKED_OUT_TEXT){
        drawString(48,16,PSTR(knocked_out_text),eightPxFont,255,NULL,ALIGN_CENTER, INPROGMEM); 
      }else if(gameover_type==CAGED_TEXT){
        drawString(48,16,PSTR(caged_text),eightPxFont,255,NULL,ALIGN_CENTER, INPROGMEM); 
      }
      if(gameover_text_id==0){
        drawString(0,32,PSTR(gameover1_text),fourPxFont,255,NULL,ALIGN_LEFT, INPROGMEM);
      }else if(gameover_text_id==1){
        drawString(0,32,PSTR(gameover2_text),fourPxFont,255,NULL,ALIGN_LEFT, INPROGMEM);
      }else if(gameover_text_id==2){
        drawString(0,32,PSTR(gameover3_text),fourPxFont,255,NULL,ALIGN_LEFT, INPROGMEM);
      }else if(gameover_text_id==3){
        drawString(0,32,PSTR(gameover4_text),fourPxFont,255,NULL,ALIGN_LEFT, INPROGMEM);
      }
      
      itoa(player.SCORE, buf, 10);
      drawString(0,0,buf,eightPxFont,255,NULL,ALIGN_LEFT, INRAM); 
      if(checkButton(TAButton2)){
        gameState=STATE_TITLE_SCREEN;
        sound_player.Play(intro_music_handle);
      }
      break;
    }
    // buffer to screen
	tiny_screen.goTo(0, 0);
	tiny_screen.setX(0, SCREEN_WIDTH - 1);
	tiny_screen.setY(0, SCREEN_HEIGHT - 1);
	tiny_screen.startData();
	tiny_screen.writeBuffer(screen_buffer, BUFFER_SIZE);
	tiny_screen.endTransfer();

  /*char text[128];
  tiny_screen.setCursor(0, 0);
  sprintf(text, "FPS %d", 1000000 / interval_time);
  tiny_screen.print(text);
  tiny_screen.setCursor(0, 12);
  sprintf(text, "time %lums", interval_time);
  tiny_screen.print(text);*/
  sound_player.Update();
}

void game_setup() {
  game_timer = 0;
  for (int8_t i = 0; i < TotalEntities; i++) {
    entities[i].creation_timer = pgm_read_byte(&odds[i * 6]);
    entities[i].row = i;
    entities[i].Type = 0;
    entities[i].Exists = false;
  }
  for (int8_t i = 0; i < TotalDarts; i++) {
    darts[i].Exists = false;
  }
  player.SCORE = 0;
  player.jump = 0;
  player.x = 0;
  player.y = 38;
  player.row = 38;
  player.frame = 0;
  player.sliding = false;
  player.slide_timer = 0;
  player.multiplier_timer = 0;
  hotsauce_timer = 3600;
  gameState = STATE_GAME_PLAYING;
}

void isEntityCollision() {
  //set bound box
  Rect playerRect = {.x = (player.x + 4), .y = (player.y + 1), .width = 16, .height = 6};

  //check entities
  for (int8_t i = 0; i < TotalEntities; i++) {
    if (entities[i].Exists == true) {
      Rect entityRect = getEntityRect(i);
      if (collide(entityRect, playerRect)) {
        if (entities[i].Type == ELDER_ENTITY) {
          if (player.sliding == false) {
            entities[i].Exists = false;
            SoundPlayer& sound_player = SoundPlayer::GetInstance();
            sound_player.Play(eat_handle);
            if(player.multiplier_timer>0){
              player.SCORE += 2;
            }else{
              player.SCORE += 1;
            }
            entities[i].creation_timer = 0;
          }
        } else if (entities[i].Type == CHILD_ENTITY) {
          if (player.sliding == false) {
            entities[i].Exists = false;
            SoundPlayer& sound_player = SoundPlayer::GetInstance();
            sound_player.Play(eat_handle);
            if(player.multiplier_timer>0){
              player.SCORE += 4;
            }else{
              player.SCORE += 2;
            }
            entities[i].creation_timer = 0;
          }
        } else if (entities[i].Type == ADULT_ENTITY) {
          if (player.sliding == false) {
            entities[i].Exists = false;
            SoundPlayer& sound_player = SoundPlayer::GetInstance();
            sound_player.Play(eat_handle);
            if(player.multiplier_timer>0){
              player.SCORE += 8;
            }else{
              player.SCORE += 4;
            }
            entities[i].creation_timer = 0;
          }
        } else if (entities[i].Type == MONKEY_ENTITY) {
          if (player.sliding == false) {
            entities[i].Exists = false;
            SoundPlayer& sound_player = SoundPlayer::GetInstance();
            sound_player.Play(eat_handle);
            if(player.multiplier_timer>0){
              player.SCORE += 16;
            }else{
              player.SCORE += 8;
            }
            entities[i].creation_timer = 0;
          }
        } else if (entities[i].Type == TRANQ_ZOO_KEEPER_ENTITY) {
          if (player.sliding == false) {
            entities[i].Exists = false;
            SoundPlayer& sound_player = SoundPlayer::GetInstance();
            sound_player.Play(eat_handle);
            if(player.multiplier_timer>0){
              player.SCORE += 32;
            }else{
              player.SCORE += 16;
            };
            entities[i].creation_timer = 0;
          }
        } else if (entities[i].Type == WATER_PUDDLE_ENTITY){ 
          if(player.jump!=1){
            player.slide_timer = 45; //player slides for 2 secs
            player.sliding = true;
          }
        } else if (entities[i].Type == CAGE_ENTITY) {
          SoundPlayer& sound_player = SoundPlayer::GetInstance();
          sound_player.Play(game_over_handle);
          gameover_type = CAGED_TEXT;
          gameover_text_id = random(4);
          if(player.SCORE>high_score_data.score){
            high_score_data.score=player.SCORE;
            Save();
          }
          gameState = STATE_GAME_OVER; //game over
        } else if (entities[i].Type == ELEPHANT_ENTITY) {
          SoundPlayer& sound_player = SoundPlayer::GetInstance();
          sound_player.Play(game_over_handle);
          gameover_type = TRAMPLED_TEXT;
          gameover_text_id = random(4);
          if(player.SCORE>high_score_data.score){
            high_score_data.score=player.SCORE;
            Save();
          }
          gameState = STATE_GAME_OVER; //game over
        } else if (entities[i].Type == HOT_SAUCE_ENTITY) {
          SoundPlayer& sound_player = SoundPlayer::GetInstance();
          sound_player.Play(eat_handle);
          player.multiplier_timer = 900;
          entities[i].Exists=false;
        }
      }
    }
  }
}

void isDartCollision() {
  Rect playerRect = {.x = (player.x + 4), .y = (player.y + 1), .width = 16, .height = 6};

  for (int8_t i = 0; i < TotalDarts; i++) {
    if (darts[i].Exists == true) {
      Rect dartRect = {.x = (darts[i].point.x + 1), .y = (darts[i].point.y + 1), .width = 6, .height = 6};
      if (darts[i].Type == MONKEY_POO) {
        Rect dartRect = {.x = darts[i].point.x, .y = darts[i].point.y, .width = 16, .height = 8};
      } 
      if (collide(dartRect, playerRect)) {
        if (darts[i].Type == MONKEY_POO) {
          player.slide_timer = 45; //player slides for 2 secs
          player.sliding = true;

        } else {
          SoundPlayer& sound_player = SoundPlayer::GetInstance();
          sound_player.Play(game_over_handle);
          gameover_type = KNOCKED_OUT_TEXT;
          gameover_text_id = random(4);
          if(player.SCORE>high_score_data.score){
            high_score_data.score=player.SCORE;
            Save();
          }
          gameState = STATE_GAME_OVER; //game over
        }
      }
    }
  }
}

Rect getEntityRect(byte i) {
  if (entities[i].Type == ELDER_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y + 8, .width = 8, .height = 8};
  } else if (entities[i].Type == CHILD_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y + 8, .width = 8, .height = 8};
  } else if (entities[i].Type == ADULT_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y + 8, .width = 8, .height = 8};
  } else if (entities[i].Type == MONKEY_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y, .width = 8, .height = 8};
  } else if (entities[i].Type == TRANQ_ZOO_KEEPER_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y + 8, .width = 8, .height = 8};
  } else if (entities[i].Type == WATER_PUDDLE_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y, .width = 16, .height = 8};
  } else if (entities[i].Type == CAGE_ENTITY) {
    return {.x = entities[i].point.x >> 3, .y = entities[i].point.y + 8, .width = 12, .height = 8};
  } else if (entities[i].Type == ELEPHANT_ENTITY) {
    return {.x = (entities[i].point.x >> 3) + 8, .y = entities[i].point.y + 24, .width = 16, .height = 8};
  } else if (entities[i].Type == HOT_SAUCE_ENTITY) {
    return {.x = (entities[i].point.x >> 3), .y = entities[i].point.y, .width = 8, .height = 8};
  }
}




void createEntity(int8_t type, int16_t X, int8_t id) {
  /*
    MONKEY_ENTITY 8x8
    WATER_PUDDLE_ENTITY 16x8
    MONKEY_POO_ENTITY 16x8
    ELDER_ENTITY 8x16
    CHILD_ENTITY 8x16
    ADULT_ENTITY 8x16
    TRANQ_ZOO_KEEPER_ENTITY 16x16
    CAGE_ENTITY 32x16
    ELEPHANT_ENTITY 32x32
    TRANQ_TRUCK_ENTITY 32x32
    row 0 = 21
    row 1 = 34
    row 2 = 46
    row 3 = 58
  */
  if (!entities[id].Exists) {
    entities[id].point.x = (X << 3);
    if ( type == TRANQ_TRUCK_ENTITY || type == ELEPHANT_ENTITY) {
      if (entities[id].row == 0) {
        entities[id].point.y = 21 - 32;
      } else {
        entities[id].point.y = entities[id].row * 12 + 22 - 32;
      }
      entities[id].width = 32;
    } else if ( type == ELDER_ENTITY || type == CHILD_ENTITY || type == ADULT_ENTITY
                || type == TRANQ_ZOO_KEEPER_ENTITY || type == CAGE_ENTITY) {
      if (entities[id].row == 0) {
        entities[id].point.y = 21 - 16;
      } else {
        entities[id].point.y = entities[id].row * 12 + 22 - 16;
      }
      entities[id].width = 8;
      if (type == CAGE_ENTITY) {
        entities[id].width = 32;
      } else if (type == TRANQ_ZOO_KEEPER_ENTITY) {
        entities[id].width = 16;
      }
    } else if ( type == MONKEY_ENTITY || type == WATER_PUDDLE_ENTITY || type == HOT_SAUCE_ENTITY ) {
      if (entities[id].row == 0) {
        entities[id].point.y = 21 - 8;
      } else {
        entities[id].point.y = entities[id].row * 12 + 22 - 8;
      }
      entities[id].width = 8;
      if (type == WATER_PUDDLE_ENTITY) {
        entities[id].width = 16;
      }
    }
    entities[id].Exists = true;
    entities[id].frame = 0;
    entities[id].Type = type;
    int8_t level = game_timer / 900;
    if (level > MAX_LEVEL) {
      level = MAX_LEVEL;
    }
    entities[id].creation_timer = pgm_read_byte(&odds[id * 6 + level * 24]);
    entities[id].darts = 1;
    if (type == 0) {
      entities[id].Exists = false;
    }
  }
}

void tickEntities(int8_t i) {
    if (entities[i].creation_timer > 0) {
      entities[i].creation_timer--;
    }
    if (entities[i].Exists == true) {
      if (entities[i].Type == ELDER_ENTITY) { //1
        //move
        entities[i].point.x -= 12;
        //set frame
        if (everyXFrames(15)) {
          entities[i].frame++;
        }
        if (entities[i].frame > 1) {
          entities[i].frame = 0;
        }
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, elder_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == CHILD_ENTITY) { //2
        //move
        entities[i].point.x -= 8;
        //set frame
        if (everyXFrames(6)) {
          entities[i].frame++;
        }
        if (entities[i].frame > 3) {
          entities[i].frame = 0;
        }
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, child_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == ADULT_ENTITY) { //3
        //move
        entities[i].point.x -= 6;
        //set frame
        if (everyXFrames(4)) {
          entities[i].frame++;
        }
        if (entities[i].frame > 3) {
          entities[i].frame = 0;
        }
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, adult_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == MONKEY_ENTITY) { //4
        //if player is right behind me then spray shit
        Rect behindRect = {.x = ((entities[i].point.x >> 3) - 24), .y = (entities[i].point.y + 1), .width = 16, .height = 6};
        Rect playerRect = {.x = (player.x + 4), .y = (player.y + 1), .width = 16, .height = 6};
        if (collide(behindRect, playerRect)) {
          createDart(MONKEY_POO, (entities[i].point.x >> 3) - 16, entities[i].point.y);
        }
        //move
        entities[i].point.x -= 7;
        //set frame
        if (everyXFrames(4)) {
          entities[i].frame++;
        }
        if (entities[i].frame > 3) {
          entities[i].frame = 0;
        }
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, monkey_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == ELEPHANT_ENTITY) { //5
        //move
        entities[i].point.x -= 17;
        //set frame
        if (everyXFrames(15)) {
          entities[i].frame++;
        }
        if (entities[i].frame > 1) {
          entities[i].frame = 0;
        }
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, elephant_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == WATER_PUDDLE_ENTITY) { //5
        //move
        entities[i].point.x -= 16;
        //set frame
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, waterpuddle_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == TRANQ_TRUCK_ENTITY) { //6
        //if player is in line of sight shoot a tranq dart
        if (entities[i].darts > 0) {
          Point first_point = {.x = ((entities[i].point.x >> 3) + 12) , .y = (entities[i].point.y + 21) };
          Point second_point = {.x = ((entities[i].point.x >> 3) + 12 - 64) , .y = (entities[i].point.y + 21 + 64) };
          drawLine(first_point.x-11, first_point.y, second_point.x-11, second_point.y, 0);
          Rect playerRect = {.x = (player.x + 4), .y = (player.y + 1), .width = 16, .height = 6};
          if (collide(first_point, second_point, playerRect)) {
            entities[i].darts--;
            createDart(ANGLED_TRANQ_DART, (entities[i].point.x >> 3) + 1, entities[i].point.y + 21);
          }
        }
        //move
        entities[i].point.x -= 4;
        //set frame
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, truck_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == CAGE_ENTITY) { //
        //move
        entities[i].point.x -= 16;
        //set frame
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, cage_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == HOT_SAUCE_ENTITY) { //
        //move
        entities[i].point.x -= 16;
        //set frame
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, hotsauce_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      } else if (entities[i].Type == TRANQ_ZOO_KEEPER_ENTITY) { //
        //if player is on the same row he will shoot 1 tranq dart, he only has 1 dart
        if (entities[i].darts > 0) {
          Point first_point = {.x = 0 , .y = (entities[i].point.y + 9) };
          Point second_point = {.x = (entities[i].point.x>>3) , .y = (entities[i].point.y + 9) };
          drawLine(first_point.x, first_point.y, second_point.x, second_point.y, 0);
          Rect playerRect = {.x = (player.x + 4), .y = (player.y), .width = 16, .height = 8};
          if (collide(first_point, second_point, playerRect)) {
            entities[i].darts--;
            createDart(TRANQ_DART, (entities[i].point.x >> 3), entities[i].point.y + 6);
          }
        }
        //move
        entities[i].point.x -= 16;
        //set frame
        //draw
        DrawSprite((entities[i].point.x >> 3), entities[i].point.y, zookeeper_plus_mask, entities[i].frame, SPRITE_WITH_MASK);
      }
      if (entities[i].point.x < -(entities[i].width << 3)) {
        entities[i].Exists = false;
      }
    } else {
      //decide if it is time to create another entity
      if (entities[i].creation_timer == 0) {
        //create new entiy
        int8_t level = game_timer / 900;
        if (level > MAX_LEVEL) {
          level = MAX_LEVEL;
        }
        if(hotsauce_timer==0 && i!=0){//create a hotsauce multiplier
          hotsauce_timer=3600;
          createEntity(HOT_SAUCE_ENTITY,128,i);
        }else{
          createEntity(pgm_read_byte(&odds[i * 6 + level * 24 + random(5) + 1]), 128, i);
        }
      }
    }
}

void createDart(int8_t type, int16_t X, int8_t Y) {
  /*
    TRANQ_DART_ENTITY 8x8
    ANGLED_TRANQ_DART_ENTITY 8x8
  */
  for (int8_t i = 0; i < TotalDarts; i++) {
    if (!darts[i].Exists) {
      darts[i].point.x = X;
      darts[i].point.y = Y;
      darts[i].Exists = true;
      darts[i].Type = type;
      break;
    }
  }
}

void tickDarts() {
  for (byte i = 0; i < TotalDarts; i++) {
    if (darts[i].Exists == true) {
      if (darts[i].Type == TRANQ_DART) { //1
        //move
        darts[i].point.x -= 4;
        //draw
        DrawSprite(darts[i].point.x, darts[i].point.y, tranqdart_plus_mask, 0,SPRITE_WITH_MASK);
      } else if (darts[i].Type == ANGLED_TRANQ_DART) { //2
        //move
        darts[i].point.x -= 2;
        darts[i].point.y += 2;
        //draw
        DrawSprite(darts[i].point.x, darts[i].point.y, angledtranqdart_plus_mask, 0, SPRITE_WITH_MASK);
      } else if (darts[i].Type == MONKEY_POO) {
        //move
        darts[i].point.x -= 16;
        //set frame
        //draw
        DrawSprite((darts[i].point.x), darts[i].point.y, monkeypoo_plus_mask, 0, SPRITE_WITH_MASK);
      }
      if (darts[i].point.x < -8) {
        darts[i].Exists = false;
      }
      if (darts[i].point.y > 64) {
        darts[i].Exists = false;
      }
    }
  }
}


bool Load() {
  // Initialize SD card
  if(sd_fat.begin(10, SPI_FULL_SPEED))
  {
    SdFile file;
    if(file.open("LEANNA/LEANNA.SAV", O_RDONLY))
    {
      unsigned int file_size = file.fileSize();      
      file_buffer = new char[file_size + 1];
      memset(file_buffer, 0, file_size + 1);
      int k = file.read(file_buffer, file_size);
      for (unsigned int t=0; t<k; t++){
        *((char*)&high_score_data + t) = file_buffer[t];
      }
      file.close();
    }
    return true;
  }else{
    return false;
  }
}

bool Save() {
  // Initialize SD card
  if(sd_fat.begin(10, SPI_CLOCK_DIV2))
  {
    SdFile file;
    if(file.open("LEANNA/LEANNA.SAV", O_WRITE | O_CREAT | O_TRUNC))
    {          
      file_buffer = new char[5];
      memcpy(file_buffer, &high_score_data, 4);
      int t = file.write(file_buffer,5);
      if(t<5){SAVE_FAILED=true;}else{SAVE_FAILED=false;}
      file.close();      
    }
  }else{
    sd_fat.initErrorHalt();
    SAVE_FAILED=true; 
  }
}

