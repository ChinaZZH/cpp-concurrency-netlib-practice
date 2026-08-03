#include "State.h"
#include "StateMachine.h"

void State::TransitionTo(StateMachine* fsm, State* new_state)
{
    if(fsm)
    {
        fsm->ChangeState(new_state);
    }
}
    