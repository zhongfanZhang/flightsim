#ifndef _INPUTSTATE_H_
#define _INPUTSTATE_H_

struct InputState
{    
	InputState(): lMousePressed(false),
                  rMousePressed(false),
                  qPressed(false),
                  ePressed(false),
                  wPressed(false),
                  sPressed(false),
                  aPressed(false),
                  dPressed(false),
                  fPressed(false),
                  bPressed(false),
                  spacePressed(false),
                  altPressed(false),
    	    	  prevX(0), prevY(0),
	    	      deltaX(0), deltaY(0) {}

    // Is the mouse button currently being held down?
	bool lMousePressed;
	bool rMousePressed;
    
    // Are buttons currently being held down
    bool qPressed;
    bool ePressed;
    bool wPressed;
    bool sPressed;
    bool aPressed;
    bool dPressed;
    bool fPressed;
    bool bPressed;
    bool spacePressed;
    bool altPressed;

    // Last known position of the cursor
	float prevX;
	float prevY;

    // Accumulated change in cursor position. 
	float deltaX;
	float deltaY;

    // Update cursor variables based on new position x,y
    void update(float x, float y)
    {
        float xDiff = x - prevX;
        float yDiff = y - prevY;
        deltaX += xDiff;
        deltaY += yDiff;
        prevX = x;
        prevY = y;
    };

    // Read off the accumulated motion and reset it
    void readDeltaAndReset(float *x, float *y)
    {
        *x = deltaX;
        *y = deltaY;
        deltaX = 0;
        deltaY = 0;
    };
};

#endif
