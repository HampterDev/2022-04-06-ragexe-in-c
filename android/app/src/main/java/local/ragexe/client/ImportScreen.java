package local.ragexe.client;

import android.app.Activity;
import android.net.Uri;
import android.os.SystemClock;
import android.text.format.Formatter;
import android.util.TypedValue;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;
import java.util.List;

/* The screen the activity shows instead of the game while the game folder holds no game, or when the
   player asks for it with the app's shortcut. It copies the game folder the player picks with the
   system's folder picker (the device's storage, a USB drive, a Chromebook's own files) into the game
   folder, then starts the game. */
final class ImportScreen {
    interface Host {
        void pickFolder();

        void startGame();
    }

    private static final long PROGRESS_INTERVAL_MS = 100;

    private final Activity activity;
    private final File folder;
    private final Host host;
    private final TextView message;
    private final ProgressBar progress;
    private final Button choose, start;
    private volatile boolean copying;
    final View view;

    ImportScreen(Activity activity, File folder, Host host) {
        this.activity = activity;
        this.folder = folder;
        this.host = host;
        int padding = dp(24);

        TextView title = new TextView(activity);
        title.setText(R.string.import_title);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        message = new TextView(activity);
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        message.setPadding(0, dp(16), 0, dp(16));
        progress = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
        progress.setMax(1000);
        progress.setVisibility(View.GONE);
        choose = new Button(activity);
        choose.setText(R.string.import_choose);
        choose.setOnClickListener(v -> host.pickFolder());
        start = new Button(activity);
        start.setText(R.string.import_start);
        start.setOnClickListener(v -> host.startGame());

        LinearLayout buttons = new LinearLayout(activity);
        buttons.setOrientation(LinearLayout.HORIZONTAL);
        buttons.addView(choose);
        buttons.addView(start);
        LinearLayout column = new LinearLayout(activity);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(padding, padding, padding, padding);
        column.addView(title);
        message.setMaxWidth(dp(560));
        column.addView(message);
        column.addView(progress, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                                               LinearLayout.LayoutParams.WRAP_CONTENT));
        column.addView(buttons);
        ScrollView scroll = new ScrollView(activity);
        scroll.setFitsSystemWindows(true);
        scroll.setFillViewport(true);
        scroll.addView(column);
        view = scroll;

        if (GameFolder.unfinished(folder))
            show(R.string.import_unfinished);
        else if (GameFolder.holdsGame(folder))
            show(R.string.import_again);
        else
            show(R.string.import_needed);
    }

    boolean copying() {
        return copying;
    }

    private int dp(int value) {
        return (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
                                              activity.getResources().getDisplayMetrics());
    }

    private String bytes(long count) {
        return Formatter.formatShortFileSize(activity, count);
    }

    private void show(int text, Object... args) {
        show(activity.getString(text, args));
    }

    private void show(String text) {
        message.setText(text);
        progress.setVisibility(copying ? View.VISIBLE : View.GONE);
        choose.setEnabled(!copying);
        start.setEnabled(!copying && GameFolder.holdsGame(folder));
    }

    private void showFromCopy(String text) {
        activity.runOnUiThread(() -> show(text));
    }

    /* The picked folder is listed, checked and copied on a thread of its own, with the screen kept on. */
    void picked(Uri tree) {
        if (copying || folder == null)
            return;
        copying = true;
        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        progress.setProgress(0);
        show(R.string.import_listing);
        new Thread(() -> copy(tree), "GameImport").start();
    }

    private void copy(Uri tree) {
        String result = null;
        try {
            List<GameFolder.Entry> files = GameFolder.list(activity.getContentResolver(), tree);
            long growth = GameFolder.growth(folder, files), free = folder.getUsableSpace();
            if (!GameFolder.holdsGame(files)) {
                result = activity.getString(R.string.import_no_data_ini);
            } else if (growth > free) {
                result = activity.getString(R.string.import_no_space, bytes(growth), bytes(free));
            } else {
                long total = GameFolder.size(files);
                long[] shown = {0};
                showFromCopy(activity.getString(R.string.import_copying, bytes(0), bytes(total)));
                GameFolder.copy(activity.getContentResolver(), tree, files, folder, (done, all) -> {
                    long now = SystemClock.uptimeMillis();
                    if (now - shown[0] < PROGRESS_INTERVAL_MS)
                        return;
                    shown[0] = now;
                    String text = activity.getString(R.string.import_copying, bytes(done), bytes(all));
                    int permille = all > 0 ? (int)Math.min(1000, done * 1000 / all) : 0;
                    activity.runOnUiThread(() -> {
                        progress.setProgress(permille);
                        message.setText(text);
                    });
                });
            }
        } catch (IOException | RuntimeException failure) {
            result = activity.getString(R.string.import_failed, String.valueOf(failure.getMessage()));
        }
        String failure = result;
        activity.runOnUiThread(() -> {
            copying = false;
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            if (failure == null)
                host.startGame();
            else
                show(failure);
        });
    }
}
