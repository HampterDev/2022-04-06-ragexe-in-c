package local.ragexe.client;

import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;
import org.libsdl.app.SDLActivity;

/* SDL's activity, which loads libSDL3.so and libmain.so and runs the client's main(). It also tells
   the client how much of the screen the on-screen keyboard covers, which SDL does not, so that the
   client can keep the edit box being typed into above it (src/core/app_sdl.c). */
public class ClientActivity extends SDLActivity {
    private static native void onKeyboardCover(int bottom);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT < 30)
            return;
        getWindow().getDecorView().setOnApplyWindowInsetsListener((View view, WindowInsets insets) -> {
            try {
                onKeyboardCover(insets.getInsets(WindowInsets.Type.ime()).bottom);
            } catch (UnsatisfiedLinkError notLoaded) {
                // The client library did not load; SDL shows why.
            }
            return view.onApplyWindowInsets(insets);
        });
    }
}
