#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <math.h>
#include <strings.h>
#include <assert.h>

typedef struct {
        int *data;
        size_t size;
        size_t cap;
} int_da;

typedef struct {
        float *data;
        size_t size;
        size_t cap;
} float_da;

#define da_append(xs,v) do { \
        if ((xs).size >= (xs).cap) { \
                size_t new_cap = (xs).cap ? (xs).cap*2 : 2; \
                void *new_data = realloc((xs).data,new_cap*sizeof(*(xs).data)); \
                if (!new_data) { \
                        perror("realloc"); \
                        exit(EXIT_FAILURE); \
                } else { \
                        (xs).data = new_data; \
                        (xs).cap = new_cap; \
                } \
        } \
        (xs).data[(xs).size++]=(v); \
        } while(0)

int add_vert(float_da *verts,
        float x, float y, float z,
        float r, float g, float b)
{
        da_append(*verts, x);
        da_append(*verts, y);
        da_append(*verts, z);
        da_append(*verts, r);
        da_append(*verts, g);
        da_append(*verts, b);
}

int gen_something(float_da *vertices, int_da *indices)
{
        const int seg = 100;

        for (int i=0; i<=seg*2; i+=2) {
                float angle = 2 * M_PI * ((float)i / (seg*2));
                float dx = cosf(angle)*0.5;
                float dz = sinf(angle)*0.5;

                int c=i/2;
                add_vert(vertices,dx,0.5,dz,  c%2,1-(c%2),0.5);
                add_vert(vertices,dx,-0.5,dz, c%2,1-(c%2),0.5);

                if (i>0) {
                        da_append(*indices,i-2);
                        da_append(*indices,i-1);
                        da_append(*indices,i);

                        da_append(*indices,i-1);
                        da_append(*indices,i);
                        da_append(*indices,i+1);
                }
        }

        return 0;
}

#define SHADER_BUF_SIZE 1024

int createShader(unsigned int *_shader, const char *path, GLenum shader_type)
{
        char buf[SHADER_BUF_SIZE]={0};
        char srcbuf[SHADER_BUF_SIZE]={0};

        FILE *file = fopen(path,"rb");

        if (file == NULL) {
                perror("fopen failed");
                return -1;
        }
        while (fgets(buf,SHADER_BUF_SIZE,file)) strcat(srcbuf,buf);
        fclose(file);

        const char* const src[] = {srcbuf};

        int  success;
        char infoLog[512];

        unsigned int shader;
        shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, src, NULL);
        glCompileShader(shader);

        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                fprintf(stderr, "Shader Compilation Failed: %s\n",infoLog);
                return -1;
        }
        *_shader=shader;
        return 0;
}

int createProgram(unsigned int *sp, unsigned int vertexShader, unsigned int fragmentShader)
{
        int success;
        char infoLog[512];

        unsigned int shaderProgram;
        shaderProgram = glCreateProgram();

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if(!success) {
                glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                fprintf(stderr, "Shader program linking failed: %s\n",infoLog);
                return -1;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        *sp=shaderProgram;
        return 0;
}

int createVBOandVAO(unsigned int *tVBO, unsigned int *tVAO, float *vertices, size_t size)
{
        unsigned int VBO,VAO;
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindVertexArray(0);

        *tVBO=VBO;
        *tVAO=VAO;
        return 0;
}

int createEBO(unsigned int *tEBO,int *indices,size_t size)
{
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
        *tEBO=EBO;
        return 0;
}

#define WIDTH 500
#define HEIGHT 500

int main()
{
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                fprintf(stderr, "SDL could not be initialized: %s\n",SDL_GetError());
                return 1;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        SDL_Window* window = SDL_CreateWindow(
                "Hello from C!",
                WIDTH,
                HEIGHT,
                SDL_WINDOW_OPENGL
        );

        if (window==NULL) {
                fprintf(stderr, "Window could not be created: %s\n",SDL_GetError());
                SDL_Quit();
                return 1;
        }

        SDL_GLContext ctx = SDL_GL_CreateContext(window);
        if (!ctx) {
                fprintf(stderr, "OpenGL context could not be created: %s\n", SDL_GetError());
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
        }

        if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
                fprintf(stderr, "Failed to initialize GLAD\n");
                return 1;
        }

        float_da verts = {0};
        int_da indices = {0};
        gen_something(&verts,&indices);
        if (verts.size == 0 || indices.size == 0) {
                SDL_Log("couldn't make something..",SDL_GetError());
                return 1;
        }

        int err;

        unsigned int vertexShader, fragmentShader;
        err=createShader(&vertexShader,"shader.vs",GL_VERTEX_SHADER);
        if (err < 0) return 1;
        err=createShader(&fragmentShader,"shader.fs",GL_FRAGMENT_SHADER);
        if (err < 0) return 1;

        unsigned int shaderProgram;
        err=createProgram(&shaderProgram,vertexShader,fragmentShader);
        if (err < 0) return 1;

        unsigned int VBO,VAO;
        createVBOandVAO(&VBO,&VAO,verts.data,verts.size*sizeof(*verts.data));

        unsigned int EBO;
        createEBO(&EBO,indices.data,indices.size*sizeof(*indices.data));

        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0f);

        int running = 1;
        while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_EVENT_QUIT) {
                                running = 0;
                        }
                }

                glClearColor(0.0f,0.0f,0.0f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(shaderProgram);
                glBindVertexArray(VAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glBindBuffer(GL_ARRAY_BUFFER,VBO);
                glDrawElements(GL_TRIANGLE_STRIP, indices.size, GL_UNSIGNED_INT, 0);

                SDL_GL_SwapWindow(window);
                SDL_Delay(16);
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);

        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
}
