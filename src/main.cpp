// ============================================================
//  OpenGL Ludo Game Engine
//  Data Structures Used:
//   1. Circular Linked List  -> Player turn rotation
//   2. Stack                 -> Move history (undo support)
//   3. Graph (Adjacency List)-> Board path representation
//   4. 2D Array / Matrix     -> Board cell layout mapping
//   5. Arrays                -> Token positions per player
// ============================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>

// ============================================================
//  SECTION 1: BOARD GRAPH
// ============================================================
struct GraphNode { int cellId; bool isSafe; int nextCell; };
GraphNode boardGraph[52];

void initBoardGraph() {
    for (int i = 0; i < 52; i++) {
        boardGraph[i].cellId   = i;
        boardGraph[i].nextCell = (i + 1) % 52;
        boardGraph[i].isSafe   = false;
    }
    // Sahi track indices for 4 Colored Starts + 4 Grey Safe Zones
    int sc[] = {2, 10, 15, 23, 28, 36, 41, 50};
    for (int i = 0; i < 8; i++) boardGraph[sc[i]].isSafe = true;
}

bool isSafeCell(int a) {
    if (a < 0 || a >= 52) return false;
    return boardGraph[a].isSafe;
} 

// ============================================================
//  SECTION 2: CIRCULAR LINKED LIST
// ============================================================
struct PlayerNode {
    int playerId; std::string name; float color[3]; PlayerNode* next;
};

PlayerNode* buildPlayerCycle() {
    PlayerNode* p1=new PlayerNode(); PlayerNode* p2=new PlayerNode();
    PlayerNode* p3=new PlayerNode(); PlayerNode* p4=new PlayerNode();
    p1->playerId=1; p1->name="BLUE";   p1->color[0]=0.16f;p1->color[1]=0.50f;p1->color[2]=0.83f;
    p2->playerId=2; p2->name="RED";    p2->color[0]=0.85f;p2->color[1]=0.23f;p2->color[2]=0.19f;
    p3->playerId=3; p3->name="GREEN";  p3->color[0]=0.15f;p3->color[1]=0.68f;p3->color[2]=0.37f;
    p4->playerId=4; p4->name="YELLOW"; p4->color[0]=0.95f;p4->color[1]=0.77f;p4->color[2]=0.06f;
    p1->next=p2; p2->next=p3; p3->next=p4; p4->next=p1;
    return p1;
}

PlayerNode* nextPlayer(PlayerNode* c) { return c->next; }

// ============================================================
//  SECTION 3: STACK
// ============================================================
struct MoveRecord { int playerId,tokenIdx,prevPosition,newPosition; };
struct MoveStack {
    MoveRecord data[256]; int top;
    MoveStack(){top=-1;}
    bool isEmpty(){return top==-1;} bool isFull(){return top==255;}
    void push(MoveRecord r){if(!isFull())data[++top]=r;}
    MoveRecord pop(){if(!isEmpty())return data[top--];return{-1,-1,-1,-1};}
    MoveRecord peek(){if(!isEmpty())return data[top];return{-1,-1,-1,-1};}
};

// ============================================================
//  SECTION 4: 2D MATRIX
// ============================================================
int boardMatrix[15][15];
void initBoardMatrix() {
    for(int r=0;r<15;r++) for(int c=0;c<15;c++) boardMatrix[r][c]=0;
    for(int c=1;c<=5;c++) boardMatrix[7][c]=1; boardMatrix[8][1]=1;
    for(int r=9;r<=13;r++) boardMatrix[r][7]=2; boardMatrix[13][8]=2;
    for(int c=9;c<=13;c++) boardMatrix[7][c]=3; boardMatrix[6][13]=3;
    for(int r=1;r<=5;r++) boardMatrix[r][7]=4; boardMatrix[1][6]=4;
    int sR[]={6,8,2,12,6,8,2,12}, sC[]={2,12,6,8,12,2,8,6};
    for(int i=0;i<8;i++) boardMatrix[sR[i]][sC[i]]=9;
}

// ============================================================
//  SECTION 5: TOKEN ARRAYS
// ============================================================
int p1Tokens[4]={-1,-1,-1,-1};
int p2Tokens[4]={-1,-1,-1,-1};
int p3Tokens[4]={-1,-1,-1,-1};
int p4Tokens[4]={-1,-1,-1,-1};

int* getTokenArray(int pid){
    if(pid==1)return p1Tokens; if(pid==2)return p2Tokens;
    if(pid==3)return p3Tokens; return p4Tokens;
}

int toAbsolute(int pid,int rel){
    int off[5]={0,2,15,28,41}; return(off[pid]+rel)%52;
}

bool checkAndKillTokens(int atk,int rel){
    if(rel<0||rel>=51)return false;
    int ap=toAbsolute(atk,rel);
    if(isSafeCell(ap))return false;
    bool killed=false;
    for(int v=1;v<=4;v++){
        if(v==atk)continue;
        int* vt=getTokenArray(v);
        for(int i=0;i<4;i++){
            if(vt[i]<0||vt[i]>=51)continue;
            if(toAbsolute(v,vt[i])==ap){
                vt[i]=-1;
                std::cout<<"*** KILL! Player "<<atk<<" killed Player "<<v<<" token ["<<i<<"]! ***\n";
                killed=true;
            }
        }
    }
    return killed;
}

// ============================================================
//  SECTION 6: CONSTANTS + SHADERS + COLORS
// ============================================================
const int WIN_W = 1200;
const int WIN_H = 800;

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec2 offset; uniform vec2 scale;
void main(){ gl_Position=vec4(aPos*scale+offset,0.0,1.0); }
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor; uniform vec3 color;
void main(){ FragColor=vec4(color,1.0); }
)";

const float RED[3]    ={0.85f,0.23f,0.19f};
const float GREEN[3]  ={0.15f,0.68f,0.37f};
const float YELLOW[3] ={0.95f,0.77f,0.06f};
const float BLUE[3]   ={0.16f,0.50f,0.83f};
const float WHITE[3]  ={1.0f,1.0f,1.0f};
const float BLACK[3]  ={0.0f,0.0f,0.0f};
const float GREY[3]   ={0.70f,0.70f,0.70f};
const float GOLD[3]   ={0.85f,0.68f,0.40f};
const float BG[3]     ={0.15f,0.18f,0.22f};
const float ORANGE[3] ={1.0f,0.55f,0.0f};

// ============================================================
//  SECTION 7: GAME STATE GLOBALS
// ============================================================
PlayerNode* currentPlayer  = nullptr;
MoveStack   moveHistory;
int  diceValue       = 1;
int  finalDiceValue  = 1; 
int  turnState       = 0;
int  rollTimer       = 0;
bool clicked         = false;
bool waitingForToken = false;
std::vector<int> eligibleTokens;

void onMouseClick(GLFWwindow*,int btn,int action,int){
    if(btn==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS) clicked=true;
}

void onKeyPress(GLFWwindow*,int key,int,int action,int){
    if(key==GLFW_KEY_U&&action==GLFW_PRESS){
        if(!moveHistory.isEmpty()){
            MoveRecord last=moveHistory.pop();
            int* tk=getTokenArray(last.playerId);
            tk[last.tokenIdx]=last.prevPosition;
            std::cout<<"Undo! Player "<<last.playerId<<" token["<<last.tokenIdx<<"] restored.\n";
        }
    }
}
int rollDice(){ return(rand()%6)+1; }

// ============================================================
//  SECTION 8: BOARD COORDINATE RESOLVER
// ============================================================
struct BoardPos { int r,c; };

BoardPos getTokenCoordinates(int pid,int rel){
    if(rel==56)return{7,7};
    if(rel>=51){int s=rel-51;
        if(pid==1)return{7,1+s}; if(pid==2)return{13-s,7};
        if(pid==3)return{7,13-s}; return{1+s,7};
    }
    static const BoardPos track[52]={
        {7,0},{8,0},{8,1},{8,2},{8,3},{8,4},{8,5},
        {9,6},{10,6},{11,6},{12,6},{13,6},{14,6},{14,7},
        {14,8},{13,8},{12,8},{11,8},{10,8},{9,8},
        {8,9},{8,10},{8,11},{8,12},{8,13},{8,14},{7,14},
        {6,14},{6,13},{6,12},{6,11},{6,10},{6,9},
        {5,8},{4,8},{3,8},{2,8},{1,8},{0,8},{0,7},
        {0,6},{1,6},{2,6},{3,6},{4,6},{5,6},
        {6,5},{6,4},{6,3},{6,2},{6,1},{6,0}
    };
    int off[5]={0,2,15,28,41};
    return track[(off[pid]+rel)%52];
}

BoardPos getHomeYardPos(int pid,int idx){
    int cols[]={1,3,1,3},rows[]={1,1,3,3};
    if(pid==1)return{9+rows[idx],  cols[idx]};
    if(pid==2)return{9+rows[idx],9+cols[idx]};
    if(pid==3)return{  rows[idx],9+cols[idx]};
    return{rows[idx],cols[idx]};
}

// ============================================================
//  SECTION 9: DRAW HELPERS
// ============================================================
void drawQuad(GLint oL,GLint sL,GLint cL,
              float x,float y,float sx,float sy,const float col[3]){
    glUniform2f(oL,x,y); glUniform2f(sL,sx,sy);
    glUniform3f(cL,col[0],col[1],col[2]); glDrawArrays(GL_TRIANGLES,0,6);
}
void drawDot(GLint o,GLint s,GLint c,float x,float y,const float col[3]){
    drawQuad(o,s,c,x,y,0.025f,0.025f,col);
}
void drawDiceFace(GLint o,GLint s,GLint c,float x,float y,int n,const float fc[3]){
    drawQuad(o,s,c,x,y,0.25f,0.25f,BLACK);
    drawQuad(o,s,c,x+.005f,y+.005f,0.24f,0.24f,fc);
    float cx=x+.11f,cy=y+.11f,sp=.06f;
    switch(n){
        case 1:drawDot(o,s,c,cx,cy,WHITE);break;
        case 2:drawDot(o,s,c,cx-sp,cy+sp,WHITE);drawDot(o,s,c,cx+sp,cy-sp,WHITE);break;
        case 3:drawDot(o,s,c,cx-sp,cy+sp,WHITE);drawDot(o,s,c,cx,cy,WHITE);drawDot(o,s,c,cx+sp,cy-sp,WHITE);break;
        case 4:drawDot(o,s,c,cx-sp,cy+sp,WHITE);drawDot(o,s,c,cx+sp,cy+sp,WHITE);
               drawDot(o,s,c,cx-sp,cy-sp,WHITE);drawDot(o,s,c,cx+sp,cy-sp,WHITE);break;
        case 5:drawDot(o,s,c,cx-sp,cy+sp,WHITE);drawDot(o,s,c,cx+sp,cy+sp,WHITE);
               drawDot(o,s,c,cx,cy,WHITE);
               drawDot(o,s,c,cx-sp,cy-sp,WHITE);drawDot(o,s,c,cx+sp,cy-sp,WHITE);break;
        case 6:drawDot(o,s,c,cx-sp,cy+sp,WHITE);drawDot(o,s,c,cx+sp,cy+sp,WHITE);
               drawDot(o,s,c,cx-sp,cy,WHITE);drawDot(o,s,c,cx+sp,cy,WHITE);
               drawDot(o,s,c,cx-sp,cy-sp,WHITE);drawDot(o,s,c,cx+sp,cy-sp,WHITE);break;
    }
}
void drawGlyphP(GLint o,GLint s,GLint c,float x,float y,float sz){
    drawQuad(o,s,c,x,y,sz*.2f,sz,WHITE);
    drawQuad(o,s,c,x,y+sz*.6f,sz*.6f,sz*.2f,WHITE);
    drawQuad(o,s,c,x,y+sz*.3f,sz*.6f,sz*.2f,WHITE);
    drawQuad(o,s,c,x+sz*.5f,y+sz*.3f,sz*.2f,sz*.5f,WHITE);
}
void drawGlyphNum(GLint o,GLint s,GLint c,int n,float x,float y,float sz){
    if(n==1){drawQuad(o,s,c,x+sz*.3f,y,sz*.2f,sz,WHITE);return;}
    if(n==2){drawQuad(o,s,c,x,y+sz*.8f,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x+sz*.5f,y+sz*.4f,sz*.2f,sz*.5f,WHITE);
             drawQuad(o,s,c,x,y+sz*.3f,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x,y+sz*.1f,sz*.2f,sz*.4f,WHITE);
             drawQuad(o,s,c,x,y,sz*.7f,sz*.2f,WHITE);return;}
    if(n==3){drawQuad(o,s,c,x,y+sz*.8f,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x,y+sz*.4f,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x,y,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x+sz*.5f,y,sz*.2f,sz,WHITE);return;}
    if(n==4){drawQuad(o,s,c,x,y+sz*.4f,sz*.2f,sz*.6f,WHITE);
             drawQuad(o,s,c,x,y+sz*.4f,sz*.7f,sz*.2f,WHITE);
             drawQuad(o,s,c,x+sz*.5f,y,sz*.2f,sz,WHITE);}
}
void drawLabel(GLint o,GLint s,GLint c,int pn,float x,float y,float sz){
    drawGlyphP(o,s,c,x,y,sz); drawGlyphNum(o,s,c,pn,x+sz*.8f,y,sz);
}
void drawPlayerIndicator(GLint o,GLint s,GLint c,PlayerNode* p,float x,float y){
    drawQuad(o,s,c,x,y,.45f,.18f,BLACK);
    drawQuad(o,s,c,x+.005f,y+.005f,.44f,.17f,p->color);
    drawLabel(o,s,c,p->playerId,x+.12f,y+.04f,.09f);
}

// ============================================================
//  SECTION 10: MAIN
// ============================================================
int main(){
    srand((unsigned)time(0));
    initBoardGraph(); initBoardMatrix();
    currentPlayer = buildPlayerCycle();

    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win=glfwCreateWindow(WIN_W,WIN_H,"Ludo - Data Structures + OpenGL",NULL,NULL);
    if(!win){glfwTerminate();return -1;}
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetMouseButtonCallback(win,onMouseClick);
    glfwSetKeyCallback(win,onKeyPress);

    GLuint vs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,NULL); glCompileShader(vs);
    GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,NULL); glCompileShader(fs);
    GLuint sh=glCreateProgram();
    glAttachShader(sh,vs); glAttachShader(sh,fs); glLinkProgram(sh);
    glDeleteShader(vs); glDeleteShader(fs);

    float sqV[]={0,0,1,0,1,1,1,1,0,1,0,0};
    float triV[]={0,0,1,0,.5f,.5f, 0,1,1,1,.5f,.5f, 0,0,0,1,.5f,.5f, 1,0,1,1,.5f,.5f};
    GLuint VAO,VBO,tVAO,tVBO;
    glGenVertexArrays(1,&VAO);glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(sqV),sqV,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);glEnableVertexAttribArray(0);
    glGenVertexArrays(1,&tVAO);glGenBuffers(1,&tVBO);
    glBindVertexArray(tVAO);glBindBuffer(GL_ARRAY_BUFFER,tVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(triV),triV,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);glEnableVertexAttribArray(0);

    GLint oL=glGetUniformLocation(sh,"offset");
    GLint sL=glGetUniformLocation(sh,"scale");
    GLint cL=glGetUniformLocation(sh,"color");

    float gs=2.0f/15.0f, bsX=0.66f, shX=-0.33f;
    float dMinX=0.45f,dMaxX=0.85f,dMinY=0.15f,dMaxY=0.55f;

    std::cout<<"=== Ludo Started! Press U to undo. ===\n";
    std::cout<<currentPlayer->name<<"'s turn. Click the dice area!\n";

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();

        // ---- INPUT ----
        if(clicked){
            clicked=false;
            double mx,my; glfwGetCursorPos(win,&mx,&my);
            float nx=(float)(mx/WIN_W)*2.0f-1.0f;
            float ny=1.0f-(float)(my/WIN_H)*2.0f;
            bool dHit=(nx>dMinX&&nx<dMaxX&&ny>dMinY&&ny<dMaxY);

            if(turnState==0&&dHit){
                finalDiceValue = rollDice(); 
                turnState=1; rollTimer=15;
            }
            else if(turnState==2){
                float bx=(nx-shX)/bsX;
                int col=(int)((bx+1.0f)/gs);
                int row=(int)((ny+1.0f)/gs);
                if(col>=0&&col<15&&row>=0&&row<15){
                    int* tk=getTokenArray(currentPlayer->playerId);
                    int sel=-1;
                    for(int idx:eligibleTokens){
                        BoardPos pos=(tk[idx]==-1)
                            ?getHomeYardPos(currentPlayer->playerId,idx)
                            :getTokenCoordinates(currentPlayer->playerId,tk[idx]);
                        if(pos.r==row&&pos.c==col){sel=idx;break;}
                    }
                    if(sel!=-1){
                        int prev=tk[sel];
                        diceValue = finalDiceValue; 
                        if(tk[sel]==-1&&diceValue==6){
                            tk[sel]=0;
                            std::cout<<currentPlayer->name<<" token["<<sel<<"] entered board!\n";
                        } else {
                            tk[sel]+=diceValue;
                            std::cout<<currentPlayer->name<<" token["<<sel<<"] moved +"<<diceValue<<"\n";
                        }
                        MoveRecord rec={currentPlayer->playerId,sel,prev,tk[sel]};
                        moveHistory.push(rec);
                        eligibleTokens.clear(); waitingForToken=false;
                        bool killed=checkAndKillTokens(currentPlayer->playerId,tk[sel]);
                        if(diceValue==6){
                            std::cout<<"Rolled 6! "<<currentPlayer->name<<" bonus turn!\n";
                            turnState=0;
                        } else if(killed){
                            std::cout<<"Kill bonus! "<<currentPlayer->name<<" rolls again!\n";
                            turnState=0;
                        } else {
                            currentPlayer=nextPlayer(currentPlayer);
                            std::cout<<">>> "<<currentPlayer->name<<"'s turn. Click dice!\n";
                            turnState=0;
                        }
                        std::cout.flush();
                    }
                }
            }
            else if(turnState==3&&dHit){
                currentPlayer=nextPlayer(currentPlayer);
                std::cout<<">>> "<<currentPlayer->name<<"'s turn. Click dice!\n";
                std::cout.flush(); turnState=0;
            }
        }

        // ---- DICE ANIMATION ----
        if(turnState==1){
            diceValue=rollDice(); rollTimer--;
            if(rollTimer<=0){
                diceValue = finalDiceValue; 
                std::cout<<"\n>>> "<<currentPlayer->name<<" rolled: "<<diceValue<<"\n";
                eligibleTokens.clear();
                for(int i=0;i<4;i++){
                    if(diceValue==6&&tk[i]==-1) { int* tk=getTokenArray(currentPlayer->playerId); if(tk[i]==-1) eligibleTokens.push_back(i); }
                    else { int* tk=getTokenArray(currentPlayer->playerId); if(tk[i]>=0&&tk[i]+diceValue<=56) eligibleTokens.push_back(i); }
                }
                if(eligibleTokens.empty()){
                    std::cout<<"No moves. Click dice area to pass.\n";
                    waitingForToken=false; turnState=3;
                } else {
                    std::cout<<"Click your highlighted token to move.\n";
                    waitingForToken=true; turnState=2;
                }
                std::cout.flush();
            }
        }

        // ---- RENDERING ----
        glClearColor(BG[0],BG[1],BG[2],1.0f); glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(sh); glBindVertexArray(VAO);

        // 1. Grid
        for(int r=0;r<15;r++) for(int c=0;c<15;c++){
            float x=(-1.0f+c*gs)*bsX+shX, y=-1.0f+r*gs;
            drawQuad(oL,sL,cL,x,y,gs*bsX,gs,BLACK);
            drawQuad(oL,sL,cL,x+.001f,y+.002f,gs*bsX-.002f,gs-.004f,WHITE);
        }

        // 2. Home yards
        auto yard=[&](float bx,float by,const float col[3],int pn){
            drawQuad(oL,sL,cL,bx,by,6*gs*bsX,6*gs,col);
            drawQuad(oL,sL,cL,bx+.5f*gs*bsX,by+.5f*gs,5*gs*bsX,5*gs,WHITE);
            drawQuad(oL,sL,cL,bx+.6f*gs*bsX,by+.6f*gs,4.8f*gs*bsX,4.8f*gs,col);
            drawLabel(oL,sL,cL,pn,bx+2*gs*bsX,by+4.5f*gs,.06f);
        };
        yard((-1.0f)*bsX+shX,    1.0f-6*gs, BLUE,  1);
        yard((1.0f-6*gs)*bsX+shX,1.0f-6*gs, RED,   2);
        yard((-1.0f)*bsX+shX,    -1.0f,     YELLOW,4);
        yard((1.0f-6*gs)*bsX+shX,-1.0f,     GREEN, 3);

        // 3. Runways
        for(int c=1;c<6;c++) drawQuad(oL,sL,cL,(-1+c*gs)*bsX+shX,-1+7*gs,gs*bsX,gs,BLUE);
        drawQuad(oL,sL,cL,(-1+1*gs)*bsX+shX,-1+8*gs,gs*bsX,gs,BLUE);
        drawQuad(oL,sL,cL,(-1+2*gs)*bsX+shX,-1+6*gs,gs*bsX,gs,GREY);
        for(int r=9;r<14;r++) drawQuad(oL,sL,cL,(-1+7*gs)*bsX+shX,-1+r*gs,gs*bsX,gs,RED);
        drawQuad(oL,sL,cL,(-1+8*gs)*bsX+shX,-1+13*gs,gs*bsX,gs,RED);
        drawQuad(oL,sL,cL,(-1+6*gs)*bsX+shX,-1+12*gs,gs*bsX,gs,GREY);
        for(int c=9;c<14;c++) drawQuad(oL,sL,cL,(-1+c*gs)*bsX+shX,-1+7*gs,gs*bsX,gs,GREEN);
        drawQuad(oL,sL,cL,(-1+13*gs)*bsX+shX,-1+6*gs,gs*bsX,gs,GREEN);
        drawQuad(oL,sL,cL,(-1+12*gs)*bsX+shX,-1+8*gs,gs*bsX,gs,GREY);
        for(int r=1;r<6;r++) drawQuad(oL,sL,cL,(-1+7*gs)*bsX+shX,-1+r*gs,gs*bsX,gs,YELLOW);
        drawQuad(oL,sL,cL,(-1+6*gs)*bsX+shX,-1+1*gs,gs*bsX,gs,YELLOW);
        drawQuad(oL,sL,cL,(-1+8*gs)*bsX+shX,-1+2*gs,gs*bsX,gs,GREY);

        // 4. Central hub
        glBindVertexArray(tVAO);
        glUniform2f(oL,(-1+6*gs)*bsX+shX,-1+6*gs);
        glUniform2f(sL,3*gs*bsX,3*gs);
        glUniform3fv(cL,1,RED);    glDrawArrays(GL_TRIANGLES,3,3);
        glUniform3fv(cL,1,YELLOW); glDrawArrays(GL_TRIANGLES,0,3);
        glUniform3fv(cL,1,GREEN);  glDrawArrays(GL_TRIANGLES,9,3);
        glUniform3fv(cL,1,BLUE);   glDrawArrays(GL_TRIANGLES,6,3);
        glBindVertexArray(VAO);

        // 5. TOKENS
        double t = glfwGetTime();
        float  bounce = (float)sin(t * 6.28) * 0.007f;

        int* allTk[5]={nullptr,p1Tokens,p2Tokens,p3Tokens,p4Tokens};
        const float* tCol[5]={nullptr,BLUE,RED,GREEN,YELLOW};

        for(int p=1;p<=4;p++){
            for(int i=0;i<4;i++){
                BoardPos pos=(allTk[p][i]==-1)
                    ?getHomeYardPos(p,i)
                    :getTokenCoordinates(p,allTk[p][i]);
                float tx=(-1.0f+pos.c*gs)*bsX+shX;
                float ty=-1.0f+pos.r*gs;

                bool isEligible=false;
                if(turnState==2 && p==currentPlayer->playerId){
                    for(int idx:eligibleTokens)
                        if(idx==i){isEligible=true;break;}
                }

                if(isEligible){
                    float by=ty+bounce;
                    drawQuad(oL,sL,cL,tx-0.004f, by-0.004f,0.90f*gs*bsX, 0.90f*gs, WHITE);
                    drawQuad(oL,sL,cL,tx, by,0.78f*gs*bsX, 0.78f*gs, ORANGE);
                    drawQuad(oL,sL,cL,tx+0.004f, by+0.004f,0.65f*gs*bsX, 0.65f*gs, GOLD);
                    drawQuad(oL,sL,cL,tx+0.009f, by+0.009f,0.50f*gs*bsX, 0.50f*gs, tCol[p]);
                } else {
                    drawQuad(oL,sL,cL,tx+.005f,ty+.005f,.7f*gs*bsX,.7f*gs,GOLD);
                    drawQuad(oL,sL,cL,tx+.009f,ty+.009f,.5f*gs*bsX,.5f*gs,tCol[p]);
                }
            }
        }

        // 6. Control panel
        if(waitingForToken)
            drawQuad(oL,sL,cL,.585f,.235f,.28f,.28f,GOLD);
        
        // DICE REMOVED VISUALLY: drawDiceFace call has been commented out to prevent rendering.
        // drawDiceFace(oL,sL,cL,.60f,.25f,diceValue,currentPlayer->color);
        
        drawPlayerIndicator(oL,sL,cL,currentPlayer,.44f,.60f);

        glfwSwapBuffers(win);
    }

    // Cleanup
    glDeleteVertexArrays(1,&VAO);glDeleteBuffers(1,&VBO);
    glDeleteVertexArrays(1,&tVAO);glDeleteBuffers(1,&tVBO);
    glDeleteProgram(sh);
    PlayerNode* cur=currentPlayer;
    for(int i=0;i<4;i++){PlayerNode* t=cur;cur=cur->next;delete t;}
    glfwTerminate();
    return 0;
}