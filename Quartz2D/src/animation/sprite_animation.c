#include <animation/sprite_animation.h>

void animation_state_update(animation_state* state, f32 delta_time) {
    if (!state || !state->animation || state->animation->frame_count == 0) return;

    state->time_accumulator += delta_time;
    animation_frame* current = &state->animation->frames[state->current_frame];

    if (state->time_accumulator >= current->duration) {
        state->time_accumulator -= current->duration;
        state->current_frame++;

        if (state->current_frame >= state->animation->frame_count) {
            state->current_frame = state->animation->looping ? 0 : (state->animation->frame_count - 1);
        }
    }
}