#ifndef VAO_H
#define VAO_H

#include <glad/glad.h>

class MyVAO
{
public:
    unsigned int ID;
    MyVAO()
    {
        glGenVertexArrays(1, &ID);
    }

    //绑定VAO
    void bind()
    {
        glBindVertexArray(ID);
    }
    //解绑VAO
    void unbind()
    {
        glBindVertexArray(0);
    }
    //设置顶点属性指针
    void SetVertexAttribPointer(unsigned int index, int size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)
    {
        glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        glEnableVertexAttribArray(index);
    }

};

#endif