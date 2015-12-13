package at.bitfire.gfxtablet;

import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class NetEvent {
    enum Type {
        TYPE_MOTION((byte)0),
        TYPE_BUTTON((byte)1),
        TYPE_RELMOTION((byte)2),
        TYPE_RELBUTTON((byte)3),

        // not specified in protocol, only needed to shut down network thread
        TYPE_DISCONNECT((byte)255);
        public final byte proto_value;
        Type(byte pv) {
            proto_value = pv;
        }
    }
    static final String signature = "GfxTablet";
    static final short protocol_version = 2;

    final Type type;
    short x, y, pressure;
    byte button, button_down;


    public NetEvent(Type type) {
        this.type = type;
    }

    public NetEvent(Type type, short x, short y, short pressure) {
        this.type = type;
        this.x = x;
        this.y = y;
        this.pressure = pressure;
    }

    public NetEvent(Type type, short x, short y, short pressure, int button, boolean button_down) {
        this(type, x, y, pressure);
        this.button = (byte)button;
        this.button_down = (byte)(button_down ? 1 : 0);
    }

    public byte[] toByteArray() {
        if (type == Type.TYPE_DISCONNECT)
            return null;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(baos);

        try {
            dos.writeBytes(signature);
            dos.writeShort(protocol_version);
            dos.writeByte(type.proto_value);
            dos.writeShort(x);
            dos.writeShort(y);
            dos.writeShort(pressure);

            if (type == Type.TYPE_BUTTON || type == Type.TYPE_RELBUTTON) {
                dos.writeByte(button);
                dos.writeByte(button_down);
            }
        } catch(IOException e) {
            Log.wtf(signature, "Couldn't generate network packet");
        }

        return baos.toByteArray();
    }
}
