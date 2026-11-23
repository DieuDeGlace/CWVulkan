package org.cubevulkan.app;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.content.SharedPreferences;
import android.util.AtomicFile;
import android.util.Log;

import java.io.EOFException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

// After a crash, the next start writes log_crash.txt in the app's folder: the last unexpected
// exit and, when Android has one, the crashing thread's backtrace. (From the Infinite Cubes
// Android port, our own code.)
final class CrashLog {
    private static final int MAX_TOMBSTONE_BYTES = 2 * 1024 * 1024;
    private static final int MAX_ANR_TRACE_BYTES = 256 * 1024;
    private static final int REPORT_VERSION = 2;
    private static final int MAX_FRAMES = 20;

    private CrashLog() {}

    static void collect(Context context) {
        try {
            ActivityManager manager = context.getSystemService(ActivityManager.class);
            File directory = context.getExternalFilesDir(null);
            if (manager == null || directory == null)
                return;

            File file = new File(directory, "log_crash.txt");
            SharedPreferences settings = context.getSharedPreferences("crash_report", Context.MODE_PRIVATE);
            for (ApplicationExitInfo exit : manager.getHistoricalProcessExitReasons(context.getPackageName(), 0, 8)) {
                String reason = crashReason(exit.getReason());
                if (reason == null)
                    continue;
                if (exit.getTimestamp() > file.lastModified() ||
                        settings.getInt("report_version", 0) < REPORT_VERSION) {
                    writeReport(file, exit, reason);
                    settings.edit().putInt("report_version", REPORT_VERSION).apply();
                }
                return;
            }
        } catch (IOException | RuntimeException error) {
            Log.w("CubeVulkan", "Could not collect the previous crash", error);
        }
    }

    private static String crashReason(int reason) {
        switch (reason) {
        case ApplicationExitInfo.REASON_CRASH_NATIVE:
            return "Native crash";
        case ApplicationExitInfo.REASON_CRASH:
            return "Java crash";
        case ApplicationExitInfo.REASON_ANR:
            return "App stopped responding";
        case ApplicationExitInfo.REASON_LOW_MEMORY:
            return "Android stopped the app under memory pressure";
        default:
            return null;
        }
    }

    private static void writeReport(File file, ApplicationExitInfo exit, String reason) throws IOException {
        StringBuilder report = new StringBuilder("Previous CubeVulkan exit\n")
                .append(reason).append(" (status ").append(exit.getStatus()).append(")\n");
        if (exit.getDescription() != null)
            report.append(exit.getDescription()).append('\n');
        report.append("Last recorded RSS ").append(exit.getRss()).append(" kB\n");
        int code = exit.getReason();
        if (code == ApplicationExitInfo.REASON_CRASH_NATIVE || code == ApplicationExitInfo.REASON_ANR) {
            try (InputStream trace = exit.getTraceInputStream()) {
                if (trace != null) {
                    if (code == ApplicationExitInfo.REASON_ANR) {
                        report.append("\nAndroid thread trace\n");
                        report.append(new String(trace.readNBytes(MAX_ANR_TRACE_BYTES), StandardCharsets.UTF_8));
                    } else {
                        appendBacktrace(report, trace.readNBytes(MAX_TOMBSTONE_BYTES));
                    }
                }
            } catch (IOException error) {
                report.append("The Android thread trace could not be read.\n");
            }
        }

        AtomicFile output = new AtomicFile(file);
        FileOutputStream stream = output.startWrite();
        try {
            stream.write(report.toString().getBytes(StandardCharsets.UTF_8));
            output.finishWrite(stream);
        } catch (IOException error) {
            output.failWrite(stream);
            throw error;
        }
    }

    // Android's debuggerd tombstone schema. Only report the crashing thread's stack.
    static void appendBacktrace(StringBuilder report, byte[] data) throws IOException {
        ByteBuffer bytes = ByteBuffer.wrap(data);
        Proto root = new Proto(bytes.duplicate());
        long crashingThread = 0;
        while (root.next()) {
            if (root.field == 6) // Tombstone.tid
                crashingThread = root.number;
            if (root.field == 14) // Tombstone.abort_message
                report.append(root.text()).append('\n');
        }
        root = new Proto(bytes);
        while (root.next()) {
            if (root.field != 16 || root.data == null) // Tombstone.threads
                continue;
            Proto entry = new Proto(root.data);
            long tid = 0;
            ByteBuffer thread = null;
            while (entry.next()) {
                if (entry.field == 1)
                    tid = entry.number;
                if (entry.field == 2)
                    thread = entry.data;
            }
            if (tid == crashingThread && thread != null) {
                appendThread(report, new Proto(thread));
                return;
            }
        }
    }

    private static void appendThread(StringBuilder report, Proto thread) throws IOException {
        int frames = 0;
        while (frames < MAX_FRAMES && thread.next()) {
            if (thread.field != 4 || thread.data == null) // Thread.current_backtrace
                continue;
            Proto frame = new Proto(thread.data);
            long pc = 0;
            String function = "";
            String library = "";
            String buildId = "";
            while (frame.next()) {
                switch (frame.field) {
                case 1: pc = frame.number; break;
                case 4: function = frame.text(); break;
                case 6: library = frame.text(); break;
                case 8: buildId = frame.text(); break;
                }
            }
            report.append('#').append(frames++).append(" pc ").append(Long.toHexString(pc)).append(' ')
                    .append(library.substring(library.lastIndexOf('/') + 1));
            if (!function.isEmpty())
                report.append(' ').append(function);
            if (!buildId.isEmpty())
                report.append(" (Build ID ").append(buildId).append(')');
            report.append('\n');
        }
    }

    private static final class Proto {
        private final ByteBuffer input;
        int field;
        long number;
        ByteBuffer data;

        Proto(ByteBuffer input) {
            this.input = input;
        }

        boolean next() throws IOException {
            if (!input.hasRemaining())
                return false;
            long tag = varint();
            field = (int)(tag >>> 3);
            number = 0;
            data = null;
            switch ((int)tag & 7) {
            case 0:
                number = varint();
                break;
            case 1:
                skip(8);
                break;
            case 2:
                long length = varint();
                if (length < 0 || length > input.remaining())
                    throw new EOFException();
                data = input.slice();
                data.limit((int)length);
                skip((int)length);
                break;
            case 5:
                skip(4);
                break;
            default:
                throw new IOException("Invalid tombstone field");
            }
            return true;
        }

        String text() {
            return data == null ? "" : StandardCharsets.UTF_8.decode(data.duplicate()).toString();
        }

        private void skip(int length) throws IOException {
            if (length > input.remaining())
                throw new EOFException();
            input.position(input.position() + length);
        }

        private long varint() throws IOException {
            long value = 0;
            for (int shift = 0; shift < 64; shift += 7) {
                if (!input.hasRemaining())
                    throw new EOFException();
                int next = input.get() & 0xff;
                value |= (long)(next & 0x7f) << shift;
                if ((next & 0x80) == 0)
                    return value;
            }
            throw new IOException("Invalid tombstone number");
        }
    }
}
