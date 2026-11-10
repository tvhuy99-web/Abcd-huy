package com.cleverraven.cataclysmdda;

import java.util.concurrent.Semaphore;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputFilter;
import android.text.InputType;
import android.widget.EditText;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

public class NativeUI {
    enum YesNoDialogResponse {
        YES,
        NO
    }

    private CataclysmDDA activity;

    NativeUI(CataclysmDDA activity) {
        this.activity = activity;
    }

    private class Popup {
        private Semaphore semaphore = new Semaphore(0, true);

        public void popup(final String message) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                            .setTitle("")
                            .setCancelable(false)
                            .setMessage(message)
                            .setNeutralButton(R.string.ok, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    semaphore.release();
                                }
                            }).create();
                    dialog.show();
                }
            });

            try {
                semaphore.acquire();
            } catch (InterruptedException ex) {
                // No-op
            }
        }
    }

    private class QueryYN {
        private Semaphore semaphore = new Semaphore(0, true);

        private YesNoDialogResponse response;

        public boolean queryYN(final String message) {
            activity.runOnUiThread(new Runnable() {
                public void run() {
                    AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                            .setTitle("")
                            .setCancelable(false)
                            .setMessage(message)
                            .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    response = YesNoDialogResponse.YES;
                                    semaphore.release();
                                }
                            })
                            .setNegativeButton(R.string.no, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    response = YesNoDialogResponse.NO;
                                    semaphore.release();
                                }
                            }).create();
                    dialog.show();
                }
            });

            try {
                semaphore.acquire();
            } catch (InterruptedException ex) {
                // No-op
            }

            return response == YesNoDialogResponse.YES;
        }
    }

    private class SingleChoiceList {
        private Semaphore semaphore = new Semaphore(0, true);

        private int singleChoiceResponse;

        private final int cancel = -1;
        private final int init = -2;

        private String[] mixOptionsText(final String[] options, final boolean[] enabled) {
            String[] result = new String[options.length];
            for (int i = 0; i < options.length; i++) {
                if (enabled[i]) {
                    result[i] = options[i];
                } else {
                    result[i] = String.format("%s [%s]", options[i], activity.getString(R.string.unavailable));
                }
            }
            return result;
        }

        public int singleChoiceList(final String text, final String[] options, final boolean[] enabled) {
            singleChoiceResponse = init;

            final String[] choices = mixOptionsText(options, enabled);

            while (singleChoiceResponse == init
                    || (singleChoiceResponse != cancel && enabled[singleChoiceResponse] == false)) {
                activity.runOnUiThread(new Runnable() {
                    public void run() {
                        AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                                .setTitle(text)
                                .setItems(choices, new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        singleChoiceResponse = which;
                                        semaphore.release();
                                    }
                                })
                                .setOnCancelListener(new DialogInterface.OnCancelListener() {
                                    public void onCancel(DialogInterface dialog) {
                                        singleChoiceResponse = cancel;
                                        semaphore.release();
                                    }
                                })
                                .create();
                        dialog.show();
                    }
                });

                try {
                    semaphore.acquire();
                } catch (InterruptedException ex) {
                    // No-op
                }

                if (singleChoiceResponse != cancel && enabled[singleChoiceResponse] == false) {
                    new Popup().popup(activity.getString(R.string.unavailableOption));
                }
            }

            return singleChoiceResponse;
        }
    }


    /**
     * Shared native Android input dialog for CDDA text and numeric fields.
     * Type 0 = text, 1 = signed integer, 2 = decimal. TalkBack can inspect
     * the real EditText and the two dialog buttons; no accessibility service
     * is started or stopped by the app. A null response means Cancel/Back.
     *
     * Called from the SDL game thread, never from the UI thread.
     */
    public String inputText(final String title, final String initial, final int type,
                            final int maxLength, final double minimum, final double maximum) {
        final Semaphore completion = new Semaphore(0, true);
        final AtomicReference<String> result = new AtomicReference<>(null);
        final AtomicBoolean completed = new AtomicBoolean(false);
        activity.runOnUiThread(() -> {
            final EditText field = new EditText(activity);
            field.setSingleLine(true);
            field.setInputType(type == 1
                    ? InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED
                    : type == 2
                    ? InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED
                        | InputType.TYPE_NUMBER_FLAG_DECIMAL
                    : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
            field.setText(initial == null ? "" : initial);
            field.setSelection(field.length());
            field.setContentDescription(title);
            if (maxLength > 0) {
                field.setFilters(new InputFilter[] { new InputFilter.LengthFilter(maxLength) });
            }
            final AlertDialog dialog = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                    .setTitle(title)
                    .setView(field)
                    .setNegativeButton(R.string.cancel, (which, button) -> {
                        if (completed.compareAndSet(false, true)) { completion.release(); }
                    })
                    .setPositiveButton(R.string.ok, null)
                    .setOnCancelListener(which -> {
                        if (completed.compareAndSet(false, true)) { completion.release(); }
                    })
                    .setOnDismissListener(which -> {
                        if (completed.compareAndSet(false, true)) { completion.release(); }
                    })
                    .create();
            dialog.setOnShowListener(ignored -> {
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(button -> {
                    String value = field.getText().toString();
                    if (type != 0) {
                        try {
                            final double parsed;
                            if (type == 1) {
                                parsed = Integer.parseInt(value.trim());
                            } else {
                                parsed = Double.parseDouble(value.trim());
                                if (!Double.isFinite(parsed)) { throw new NumberFormatException(); }
                            }
                            if ((Double.isFinite(minimum) && parsed < minimum)
                                    || (Double.isFinite(maximum) && parsed > maximum)) {
                                field.setError(activity.getString(R.string.a11yNativeInputRangeError,
                                        Double.isFinite(minimum) ? minimum : -Double.MAX_VALUE,
                                        Double.isFinite(maximum) ? maximum : Double.MAX_VALUE));
                                field.requestFocus();
                                return;
                            }
                        } catch (NumberFormatException invalid) {
                            // Preserve the draft and keep the Android keyboard open.
                            field.setError(activity.getString(R.string.a11yEditorInvalid));
                            field.requestFocus();
                            return;
                        }
                    }
                    result.set(value);
                    if (completed.compareAndSet(false, true)) { completion.release(); }
                    dialog.dismiss();
                });
            });
            dialog.show();
            field.requestFocus();
        });
        try {
            completion.acquire();
        } catch (InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            return null;
        }
        return result.get();
    }

    public void popup(final String message) {
        final Popup popup = new Popup();
        popup.popup(message);
    }

    public boolean queryYN(final String message) {
        final QueryYN queryYN = new QueryYN();
        return queryYN.queryYN(message);
    }

    public int singleChoiceList(final String text, final String[] options, final boolean[] enabled) {
        final SingleChoiceList singleChoiceList = new SingleChoiceList();
        return singleChoiceList.singleChoiceList(text, options, enabled);
    }
}
