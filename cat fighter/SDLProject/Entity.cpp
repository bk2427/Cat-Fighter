#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

Entity::Entity()
{
    position     = glm::vec3(0.0f);
    velocity     = glm::vec3(0.0f);
    acceleration = glm::vec3(0.0f);
    
    movement = glm::vec3(0.0f);
    
    speed = 0;
    modelMatrix = glm::mat4(1.0f);
}

Entity::~Entity()
{
    delete [] animation_up;
    delete [] animation_down;
    delete [] animation_left;
    delete [] animation_right;
    delete [] walking;
}

void Entity::draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index)
{
    // Step 1: Calculate the UV location of the indexed frame
    float u_coord = (float) (index % animation_cols) / (float) animation_cols;
    float v_coord = (float) (index / animation_cols) / (float) animation_rows;
    
    // Step 2: Calculate its UV size
    float width = 1.0f / (float) animation_cols;
    float height = 1.0f / (float) animation_rows;
    
    // Step 3: Just as we have done before, match the texture coordinates to the vertices
    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width, v_coord,
        u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };
    
    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };
    
    // Step 4: And render
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Activate_ai(Entity *player)
{
    switch (ai_type)
    {
        case WALKER:
            ai_walker();
            break;
            
        case GUARD:
            ai_guard(player);
            break;
            
        case JUMP:
            ai_jump();
            break;
            
        default:
            break;
    }
}

void Entity::ai_walker()
{
    if (position.x < -5.0) {
        movement.x = 0.5;
        }
    else if (position.x > -3.1) {
        movement.x = -0.5;
        }
}

void Entity::ai_guard(Entity *player){
    switch(ai_state) {
        case IDLE:
            if (glm::distance(position, player->position) < 3.0f) {
                ai_state = WALKING;
            }
            if (player->position.y < -1.0f) {
                ai_state = WALKING;
            }
            break;
            
        case WALKING:
            if(player->position.x < position.x) {
                movement = glm::vec3(-1, 0, 0);
            }
            else {
                movement = glm::vec3(1, 0, 0);
            }
            break;
            
        case ATTACKING:
            break;
    }
}

void Entity::ai_jump() {
    jump = false;
    velocity.y += jumping_power;
}

//void Entity::ai_shooter(Entity* player, Entity* bullets)
//{
//    switch (ai_state) {
//    case IDLE:
//
//        movement = glm::vec3(0.0f, 0.0f, 0.0f);
//        
//        if (ammo_count < ammo && player.position.x = 0.0f)
//        {
//            bullets[ammo_count].set_position(glm::vec3(this->position.x, this->position.y, 0));
//            bullets[ammo_count].set_activity(true);
//            bullets[ammo_count].set_speed(5.f);
//            bullets[ammo_count].set_movement(glm::vec3(-1, 0, 0));
//
//            ammo_count++;
//            count_delay = 0;
//        }
//
//        //if (glm::distance(position, player->position) < 3.0f) ai_state = ATTACKING;
//        break;
//
//    case ATTACKING:
//        break;
//
//    default:
//        break;
//    }
//}

bool Entity::areEnemiesActive(Entity *enemies, int enemyCount) {
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].isActive == true) { return true;}
    }
    return false;
}

void Entity::Update(float deltaTime, Entity *player, Entity *platforms, Entity *enemies, int platformCount, int enemyCount, Entity *bullets) {
    if (!isActive) return;
    
    collidedTop = false;
    collidedBottom = false;
    collidedRight = false;
    collidedLeft = false;
    
    if (entityType == ENEMY) {Activate_ai(player);}
    
    if (animation_indices != NULL){
       if (glm::length(movement) != 0){
           animation_time += deltaTime;
           float frames_per_second = (float) 1 / SECONDS_PER_FRAME;
           if (animation_time >= frames_per_second)
           {
               animation_time = 0.0f;
               animation_index++;

               if (animation_index >= animation_frames)
               {
                   animation_index = 0;
               }
           }
       }
   }

    if (jump) {
        jump = false;
        velocity.y += jumping_power;
    }
    
    velocity.x = movement.x * speed;
    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;
    
    CheckCollisionY(platforms, platformCount);
    CheckCollisionX(platforms, platformCount);
    
    if (collidedRight || collidedLeft) {
        collidedRight = false;
        collidedLeft = false;
    }
    
    CheckCollisionY(enemies, enemyCount);
    CheckCollisionX(enemies, enemyCount);
    
    for (int i = 0; i < enemyCount; i++) {
        Entity *enemy = &enemies[i];
        // if enemy is hit on the head/jumped on
        if(enemy->collidedTop) {
            enemy->isActive = false;
        }
        
        //if player hits the enemy on the side (r or l)
        if((collidedLeft || collidedRight) || (enemy->collidedLeft || enemy->collidedRight)) {
            if(entityType == PLAYER) {isActive = false;}
        }
    }
    
    if (areEnemiesActive(enemies, enemyCount) == false || ((collidedLeft || collidedRight) && areEnemiesActive(enemies, enemyCount))) {
        velocity = glm::vec3(0);
        acceleration = glm::vec3(0);
        movement = glm::vec3(0);
    }
  
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
}

void Entity::renderbg(ShaderProgram* program){
    program->SetModelMatrix(modelMatrix);

    float vertices[] = { -5, -5, 5, -5, 5, 5, -5, -5, 5, 5, -5, 5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, textureID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::render(ShaderProgram *program)
{
    if (!isActive) return;
    
    program->SetModelMatrix(modelMatrix);
    
    if (animation_indices != NULL)
    {
        draw_sprite_from_texture_atlas(program, textureID, animation_indices[animation_index]);
        return;
    }
    
    float vertices[]   = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = {  0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}



bool Entity::CheckCollision(Entity *other) {
    if (other == this) return false;
    if (isActive == false || other->isActive == false) return false;
    float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);
    
    if (xdist < 0 && ydist < 0) return true;
    return false;
}

void Entity::CheckCollisionY(Entity *objects, int objectCount) {
    for(int i = 0; i < objectCount; i++) {
        Entity *object = &objects[i];
        
        if(CheckCollision(object)) {
            object->movement.y = 0;
            float ydist = fabs(position.y - object->position.y);
            float penetrationY = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
            if (velocity.y > 0) {
                position.y -= penetrationY;
                velocity.y = 0;
                collidedTop = true;
                object->collidedBottom = true;
            } else if (velocity.y < 0) {
                position.y += penetrationY;
                velocity.y = 0;
                collidedBottom = true;
                object->collidedTop = true;
            }
        }
    }
}

void Entity::CheckCollisionX(Entity *objects, int objectCount) {
    for(int i = 0; i < objectCount; i++) {
        Entity *object = &objects[i];
        
        if(CheckCollision(object)) {
            object->movement.x = 0;
            float xdist = fabs(position.x - object->position.x);
            float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
            if (velocity.x > 0) {
                position.x -= penetrationX;
                velocity.x = 0;
                collidedRight = true;
                object->collidedLeft = true;
            } else if (velocity.x < 0) {
                position.x += penetrationX;
                velocity.x = 0;
                collidedLeft = true;
                object->collidedRight = true;
            }
        }
    }
}

