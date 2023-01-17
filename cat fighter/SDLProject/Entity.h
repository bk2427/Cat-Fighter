enum EntityType { PLATFORM, PLAYER, ENEMY, FIREBALL };
enum AIType     { WALKER, GUARD, JUMP };
enum AIState    { WALKING, IDLE, ATTACKING };


class Entity {
public:
    EntityType entityType;
    AIType ai_type;
    AIState ai_state;
    glm::vec3 position;
    glm::vec3 movement;
    glm::vec3 acceleration;
    glm::vec3 velocity;
    
    GLuint textureID;
    glm::mat4 modelMatrix;
    
    float width = 1.0;
    float height = 1.0;
    
    bool jump = false;
    float jumping_power = 0;
    
    float speed;
    
    bool isActive = true;
    
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedRight = false;
    bool collidedLeft = false;
    
//    animating
    static const int SECONDS_PER_FRAME = 12;
    static const int LEFT  = 0,
                     RIGHT = 1,
                     UP    = 2,
                     DOWN  = 3,
                     JUMP  = 4;
    
    int ammo = 20;
    int ammo_count = 0;
    
    int *animation_right = NULL; // move to the right
    int *animation_left  = NULL; // move to the left
    int *animation_up    = NULL; // move upwards
    int *animation_down  = NULL; // move downwards
    int *animation_jump  = NULL; // jump
    
    
    int **walking          = new int*[13] { animation_left, animation_right, animation_up, animation_down, animation_jump };
    int *animation_indices = NULL;
    int animation_frames   = 0;
    int animation_index    = 0;
    float animation_time   = 0.0f;
    int animation_cols     = 0;
    int animation_rows     = 0;

    
    Entity();
    ~Entity();
    
    bool CheckCollision(Entity *other);
    void CheckCollisionY(Entity *objects, int objectCount);
    void CheckCollisionX(Entity *objects, int objectCount);
    
    void draw_sprite_from_texture_atlas(ShaderProgram *program, GLuint texture_id, int index);
    bool areEnemiesActive(Entity *enemies, int enemyCount);
    void Update(float deltaTime, Entity *player, Entity *platforms, Entity *enemies, int platformCount, int enemyCount, Entity *bullets);
    void render(ShaderProgram *program);
    void renderbg(ShaderProgram* program);
    
    void Activate_ai(Entity *player);
    void ai_walker();
    void ai_shooter(Entity* player, Entity* bullets);
    void ai_jump();
    void ai_guard(Entity *player);
};
