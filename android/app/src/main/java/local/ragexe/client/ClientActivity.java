package local.ragexe.client;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.graphics.Insets;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Toast;
import java.util.Arrays;
import org.libsdl.app.SDLActivity;

/* SDL's activity, which loads libSDL3.so and libmain.so and runs the client's main(). While the game
   folder holds no game it shows the import screen instead of SDL's surface, so that SDL starts the
   client only once there is a game to run. It also tells the client what covers its window, which SDL
   does not: the on-screen keyboard, so that the client keeps the edit box being typed into above it,
   and the caption bar a desktop draws over a maximised window, which stays when the app hides the
   system bars, so that the scene keeps out from under it (src/core/app_sdl.c). */
public class ClientActivity extends SDLActivity implements ImportScreen.Host {
    /* The app shortcut's action (res/xml/shortcuts.xml): show the import screen even when there is a game. */
    private static final String ACTION_IMPORT = "local.ragexe.client.IMPORT";
    private static final int PICK_GAME_FOLDER = 0x5267;

    private ImportScreen importScreen;
    private WindowInsets windowInsets;
    private int[] reported;

    private static native void onWindowCover(int left, int top, int right, int bottom, int keyboard);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mBrokenLibraries)
            return;
        if (ACTION_IMPORT.equals(getIntent().getAction()) || !GameFolder.holdsGame(getExternalFilesDir(null))) {
            importScreen = new ImportScreen(this, getExternalFilesDir(null), this);
            setContentView(importScreen.view);
        }
        if (Build.VERSION.SDK_INT < 30)
            return;
        View decor = getWindow().getDecorView();
        decor.setOnApplyWindowInsetsListener((View view, WindowInsets insets) -> {
            windowInsets = insets;
            reportCover();
            return view.onApplyWindowInsets(insets);
        });
        decor.getViewTreeObserver().addOnGlobalLayoutListener(this::reportCover);
    }

    private static int overlap(int covered, int from, int size) {
        return Math.max(0, Math.min(size, covered - from));
    }

    /* How far the caption bar and the on-screen keyboard reach into SDL's surface, which moves under the
       caption when the window is maximised: on every change of the insets and of the layout. */
    private void reportCover() {
        View decor = getWindow().getDecorView();
        int[] at = new int[2], cover;
        int w, h, right, bottom;
        if (windowInsets == null || mSurface == null)
            return;
        Insets caption = windowInsets.getInsets(WindowInsets.Type.captionBar());
        Insets ime = windowInsets.getInsets(WindowInsets.Type.ime());
        mSurface.getLocationInWindow(at);
        w = mSurface.getWidth();
        h = mSurface.getHeight();
        right = decor.getWidth() - (at[0] + w);
        bottom = decor.getHeight() - (at[1] + h);
        cover = new int[] {
            overlap(caption.left, at[0], w), overlap(caption.top, at[1], h), overlap(caption.right, right, w),
            overlap(caption.bottom, bottom, h), overlap(ime.bottom, bottom, h),
        };
        if (Arrays.equals(cover, reported))
            return;
        reported = cover;
        try {
            onWindowCover(cover[0], cover[1], cover[2], cover[3], cover[4]);
        } catch (UnsatisfiedLinkError notLoaded) {
            // The client library did not load; SDL shows why.
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (ACTION_IMPORT.equals(intent.getAction()) && importScreen == null)
            Toast.makeText(this, R.string.import_leave_game, Toast.LENGTH_LONG).show();
    }

    @Override
    public void pickFolder() {
        try {
            startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), PICK_GAME_FOLDER);
        } catch (ActivityNotFoundException none) {
            Toast.makeText(this, R.string.import_no_picker, Toast.LENGTH_LONG).show();
        }
    }

    /* SDL's surface takes the import screen's place; SDL starts the client once it is ready. */
    @Override
    public void startGame() {
        importScreen = null;
        setContentView(mLayout);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != PICK_GAME_FOLDER) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        if (resultCode == RESULT_OK && data != null && data.getData() != null && importScreen != null)
            importScreen.picked(data.getData());
    }

    @Override
    public void onBackPressed() {
        if (importScreen != null && importScreen.copying())
            return;
        super.onBackPressed();
    }
}
