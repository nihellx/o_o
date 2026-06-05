#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <iostream>
#include <vector>
#include <cmath>
#include <random>

#include "./glm/glm.hpp"
#include "./glm/gtc/matrix_transform.hpp"
#include "./glm/gtc/type_ptr.hpp"



using namespace std;
using namespace glm;


//Global Variables
class Particle;
class Circle;

float offsetx , offsety; // order to accsess mouse motion
int circleCount;
int particleCount;
vector<vec3> vertices; // verticies of circle triangles
vector<Particle> particles; // particle properties
vector<Circle> circles;
bool isMouseHeld = false;
bool MouseArranger = false; // using in particle holding loop to arrange  holded and released stuff

bool projectionChanged;
vec2 aabb_center;



// Platform Functions
static void key_callback(GLFWwindow* window ,int key , int action, int scancode, int mods);
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window ,double xpos,double ypos); // mouse motion callback
static void mouse_button_callback(GLFWwindow* window, int button ,int action, int mods); // mouse button callback 



void build_circle(float radius,int vCount);

struct Core
{
    vec2 position = vec2(0.0f,-6371000.0f);
    double mass = 5.9722e24;
};


int initial_id = 0;
class Circle
{
public:
    int interactionID;


    vec2 position;
    vec2 velocity;
    float radius;
    float mass;

    Circle() 
    {
    if(initial_id == 0)
    {
    interactionID = initial_id;
    initial_id += 1;
    }
    else if (initial_id != 0)
    {
    interactionID = initial_id;
    initial_id += 1;
    }
    
    
    };
};


mt19937 gen(random_device{}());
uniform_real_distribution<float> distr(-0.07f, 1.0f);

class Particle : public Circle
{
public:
    float speed;
    vec2 spread;
    bool gravity;
    


    Particle(vec2 pos ,vec2 vel, vec2 spread, float _mass , bool gravity)
    {
    position = pos;
    velocity = vel;
    this->spread = spread;
    mass = _mass;
    
    }
};

void spawnParticles(const Circle&a,vector<Particle>& particles)
{
int count = 25; // count of particles
int id = 0;

for(int i = 0; i < count; i++)
{
    id += 1;
    float mass = a.mass * 0.1;


float spreadX = distr(gen) ;
float spreadY= distr(gen) ;

vec2 spread = vec2(spreadX,spreadY);
vec2 vel = spread;
vec2 pos = a.position + vec2(a.radius,0.0f); // offset to relative circle


bool gravity = true;

particles.emplace_back(pos,vel,spread,mass,gravity);
}

}



bool checkCollisons(const Circle &a, const Circle&b) // this collison function checks the two object that only be a circles 
{
float dx = a.position.x - b.position.x;
float dy = a.position.y - b.position.y;

float distanceSquared = dx * dx + dy * dy;
float radiusSummarize = a.radius + b.radius;

return distanceSquared <= radiusSummarize * radiusSummarize;
}



void wall_collison(Circle&object)
{
const float bounciness = 1.12f;
const float gravity = 9.81;
const float dt = 0.016;
// const float friction = object.mass * gravity * 0.5 * dt;
const float friction = 0.9f;

if(object.position.x - object.radius < -1.0f) // LEFT WALL
{
object.position.x = -1.0f + object.radius;
object.velocity.x = -object.velocity.x * bounciness;
object.velocity.y *= friction;

}
else if(object.position.x + object.radius > 1.0f)// RIGHT WALL
{
object.position.x = 1.0f - object.radius;
object.velocity.x = -object.velocity.x * bounciness;
object.velocity.y *= friction;

}
if(object.position.y + object.radius > 1.0f) // TOP
{
object.position.y = 1.0f - object.radius;
object.velocity.y = -object.velocity.y * bounciness * friction;
if(object.velocity.x > abs(0.0f))
{
object.velocity.x *=friction;
}

}
else if(object.position.y - object.radius <-1.0f)   // bottooom -.-
{
object.position.y = -1.0f + object.radius;
object.velocity.y = -object.velocity.y * bounciness;
object.velocity.x *= friction;
object.velocity.y *= friction;
}
}


vec2 gravityForce(const Circle&a,const Core&b)
{
    float G = 6.67430e-11;  
    vec2 direction = b.position - a.position;

    float distance = length(direction);

    float F = G * (a.mass * b.mass) / (distance * distance);

    direction = normalize(direction);

return F * direction;
}




void Collison(Circle&a,Circle&b)
{
vec2 normal = b.position - a.position;

float distance = length(normal);


if(distance >= a.radius + b.radius)
{
return;
}
vec2 relative_velocity = b.velocity - a.velocity;


normal = normalize(normal);

float velocityNormal = dot(relative_velocity,normal);

// they moving apart 
if(velocityNormal > 0.0f)
{
return;
}



//if they moving away from each other return
// if loss = 0.0f they stick together its not absurt that can happen in some objects but if loss between 0 < loss < 1.0f they push each other
float loss = 0.5f;

float j = -(1.0f + loss) * velocityNormal;

j /= (1.0f / a.mass) + (1.0f / b.mass);

vec2 impulse = j * normal;

a.velocity -= (impulse /a.mass);
b.velocity += (impulse /b.mass);


}



void build_circle(float radius,int vCount)
{

float angleStep = 360.0f / vCount;
int triangle_count = vCount - 2;
float particle_size = radius * 0.4;

vector<vec3> temp;

for (int i = 0; i < vCount; i++)
{
    float CurrentAngle = angleStep * i;
    float x = radius * cos(radians(CurrentAngle));
    float y = radius * sin(radians(CurrentAngle));
    float z = 0.0f;


    temp.push_back(vec3(x,y,z));
}
for (int i = 0 ;i < triangle_count;i++) // creating  triangles and adding their vertices into vertices vector 
{
vertices.push_back(temp[0]);    
vertices.push_back(temp[i+1]);
vertices.push_back(temp[i+2]);
}
}












// Window Properties and Window Functions
struct run
{
    GLFWwindow* window;
    GLuint vao , vbo;
    GLuint shader_program;



    void Shaders()
    {
            // Vertex Shader // 
        const char* vertexShaderSource = 
        "#version 410 core\n"
        "layout (location = 0) in vec3 aPos;\n"

        "uniform mat4 model;\n"
        "uniform mat4 aScale;\n"

        "void main(){\n"
            "gl_Position = aScale * model * vec4(aPos,1.0);\n"
        "}";


        const char* fragmentShaderSource = 
            // Fragment Shader //
        "#version 410 core\n"
        "out vec4 frag_color;\n"
        "uniform vec3 u_Color;\n"

        "void main()\n"
        "{\n"

        "frag_color = vec4(u_Color, 1.0);\n"
        "}";

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader,1,&vertexShaderSource,NULL);
        glCompileShader(vertex_shader);

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader,1,&fragmentShaderSource,NULL);
        glCompileShader(fragment_shader);

        shader_program = glCreateProgram();

        glAttachShader(shader_program,vertex_shader);
        glAttachShader(shader_program,fragment_shader);

        glLinkProgram(shader_program);
        glUseProgram(shader_program);

    }
    run(int width, int height)
    {
        if(!glfwInit()){
            glfwTerminate();

            return;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        window = glfwCreateWindow(width,height,"pun",NULL,NULL);

        if(!window)
        {
        cout << "Window Initilaizetion failed";
        glfwTerminate();
        return;
        }

        glfwMakeContextCurrent(window);

        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window,mouse_callback); // cursor pos callback is only for mouse motion tracking not for click
        glfwSetMouseButtonCallback(window,mouse_button_callback);


        // glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
        cout << "glad initialize failed";
        return;
        }

        glViewport(0, 0, width, height);

        glGenVertexArrays(1,&vao); // Creating Vertex Array Object vao
        glGenBuffers(1,&vbo);      // Creating Vertex Buffer Object vbo

        glBindVertexArray(vao); // bind the vao

        float radius;
        radius = 0.1;
        build_circle(radius,36);
        circleCount = vertices.size();

        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vec3)* vertices.size(),&vertices[0],GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
            

        Shaders();
        Circle circle , circle2;

        bool is_collided = false;
        bool particle_emit = false;



        circle.radius = radius;
        circle2.radius = radius;


        float maxSpeed = 7.0f;
        //starting pos
        circle.position = vec2(-1.0f,-1.0f); 
        circle2.position = vec2(1.0f,-1.0f);

        //starting vel
        circle.velocity = vec2(5.0f,10.0f);
        circle2.velocity = vec2(-5.0f,10.0f);

        //starting mass
        circle.mass = 15.0f;
        circle2.mass = 5.0f;

        radius = 0.05;

        build_circle(radius,36);
        particleCount = vertices.size() - circleCount;

        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        // we have to upload the new circles into our vbo without this it doesnt know about their size and will draw it like last time we uploaded 
        glBufferData(GL_ARRAY_BUFFER,sizeof(vec3) * vertices.size(),&vertices[0],GL_STATIC_DRAW); 
                                        



        vec2 acceleration;

        Core core;

        float i = 0.0;

        float lastTime =  glfwGetTime();
        while(!glfwWindowShouldClose(window)) // main loop
        {

            float currentTime = glfwGetTime();
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;


            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(shader_program);

            int modelLoc = glGetUniformLocation(shader_program,"model");
            int colorLoc = glGetUniformLocation(shader_program,"u_Color");
            int scaleLoc = glGetUniformLocation(shader_program,"aScale");


            vec2 gravity = gravityForce(circle,core);
            vec2 gravity2 = gravityForce(circle2,core);
            vec2 acceleration = gravity / circle.mass;
            vec2 acceleration2 = gravity2 /circle2.mass;


            circle.velocity += acceleration * deltaTime;
            circle2.velocity += acceleration2 * deltaTime;   

            if(length(circle.velocity)> maxSpeed)
            {
                circle.velocity = normalize(circle.velocity) * maxSpeed;
            }
            if(length(circle2.velocity)> maxSpeed)
            {
                circle2.velocity = normalize(circle2.velocity) * maxSpeed;
            }



            circle.position += circle.velocity * deltaTime;
            circle2.position += circle2.velocity * deltaTime;
            

        
           if (checkCollisons(circle,circle2)&& is_collided == false)
                    {
                    spawnParticles(circle,particles);
                    is_collided = true;
                    particle_emit= true;
                    // circleCount = 0; // if want to erase the big circles after collison 

      
            }


            Collison(circle,circle2);


            wall_collison(circle);
            wall_collison(circle2);
    



            mat4 model = mat4(1.0f);
            model = translate(model,vec3(circle.position,0.0f));

            mat4 scaleMatrix = mat4(1.0f);
            scaleMatrix = scale(scaleMatrix, vec3(1.0f, 1.0f, 1.0f));

            glUniformMatrix4fv(scaleLoc,1,GL_FALSE,value_ptr(scaleMatrix));
            glUniformMatrix4fv(modelLoc,1,GL_FALSE,value_ptr(model));

        
            glBindVertexArray(vao);

            glUniform3f(colorLoc,0.5f,0.2f,0.3f);

            glDrawArrays(GL_TRIANGLES,0,circleCount);


            model = mat4(1.0f);
            model = translate(model,vec3(circle2.position,0.0f));


            glUniformMatrix4fv(modelLoc,1,GL_FALSE,value_ptr(model));
            glBindVertexArray(vao);

            glUniform3f(colorLoc,0.3f,0.1f,0.3f);
            glDrawArrays(GL_TRIANGLES,0,circleCount);

            


            for(size_t i = 0 ; i < particles.size(); i++)
            {
            if (particle_emit == true)
            {
            particles[i].radius = radius;
            wall_collison(particles[i]);
            
        

            vec2 g = gravityForce(particles[i],core);

            if(particles[i].gravity == false)
            {
            if(isMouseHeld)
            {
                if(MouseArranger == false) // just a one time we have to multiply their velocity with 0 otherwise that would break the velocity
                {
                particles[i].velocity *= 0;
                MouseArranger=true; 
                }

                g *= 0; // prevent pull to core while holding 
            }

            }

            vec2 accelerat = g / particles[i].mass;
            

            particles[i].velocity += accelerat * deltaTime;
            particles[i].position += particles[i].velocity * deltaTime;


    

            if(length(particles[i].velocity) > maxSpeed)
            {
                particles[i].velocity = normalize(particles[i].velocity) * maxSpeed;
            }

            for(size_t j = i + 1; j < particles.size(); j++)
            {
            Collison(particles[i],particles[j]);
            Collison(particles[i],circle);
            Collison(particles[i],circle2);

            }
   

            mat4 p_model = mat4(1.0f);
            p_model = translate(p_model,vec3(particles[i].position,0.0f));
  
            glUniformMatrix4fv(modelLoc,1,GL_FALSE,value_ptr(p_model));

            glBindVertexArray(vao);

            glUniform3f(colorLoc,0.4f,2.0f,0.5f);

            glDrawArrays(GL_TRIANGLES,particleCount,vertices.size());
            }
        }



            glfwSwapBuffers(window);
            glfwPollEvents();
            
        }

    
        glfwDestroyWindow(window);
        glfwTerminate();
    }

        
};


int main()
{
    run Mywin(1000,1000);

    


    return 0;
}



static void key_callback(GLFWwindow* window ,int key ,  int scancode, int action, int mods) // callbacks
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
         glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS){
        for(auto&p : particles)
        {
            p.velocity += vec2(distr(gen),distr(gen)) * 2.0f;
        }
    }


}




static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

}



float lastx = 500 , lasty  = 500;


bool initialm = true; // for checking the first mouse input
static void mouse_callback(GLFWwindow* window ,double xpos,double ypos) // this is for mouse motion callback
{



if (initialm)
{
lastx = xpos;
lasty = ypos;
initialm = false;
}


offsetx = xpos - lastx;
offsety = lasty - ypos;



lastx = xpos;
lasty = ypos;


float sensitivity = 0.05;
offsetx *= sensitivity;
offsety *= sensitivity;

float nx = (xpos / 1000) * 2.0f - 1.0f;
float ny = 1.0f - (ypos / 1000) * 2.0f;

vec2 mousePos = vec2(nx,ny);

if(isMouseHeld)
{
for(auto&p : particles)
{
    vec2 distance = p.position - mousePos;
    
    if(dot(distance,distance) < p.radius * p.radius) // we are using dot product here instead of lenght sqrt is expensive its cheaper but same result 
    // in cases like u dont need exact value of return of lenght using dot product is better way
    {
    p.position = mousePos;
    // p.velocity *= 0;
    p.gravity = false;
    }
    else
    {
    p.gravity = true;
    }

}
}



}

static void mouse_button_callback(GLFWwindow* window, int button ,int action, int mods)
{
    // mouse button stuff

if (button == GLFW_MOUSE_BUTTON_LEFT)
{
    if(action == GLFW_PRESS)
    {
        isMouseHeld = true;
    }
    else if (action == GLFW_RELEASE)
    {
        isMouseHeld = false;
        MouseArranger = false;
    }

}
}