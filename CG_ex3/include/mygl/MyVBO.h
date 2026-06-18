#ifndef VBO_H
#define VBO_H

#include <glad/glad.h>

class MyVBO
{
public:
    unsigned int ID;
    MyVBO(float* vertices, size_t size)
    {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ARRAY_BUFFER, ID);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    //绑定VBO
    void bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, ID);
    }
    //解绑VBO
    void unbind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

#endif