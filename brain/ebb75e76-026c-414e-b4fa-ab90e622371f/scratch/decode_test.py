
import base64

def decode_thrillrig(input_str):
    print(f"Original: {input_str}")
    
    # 1. Read first digit n
    n = int(input_str[0])
    print(f"n = {n}")
    
    # 2. Substring(n + 1)
    line = input_str[n+1:]
    print(f"After substr({n+1}): {line}")
    
    # 3. Swap front and back
    line_list = list(line)
    line_list[0], line_list[-1] = line_list[-1], line_list[0]
    line = "".join(line_list)
    print(f"After swap: {line}")
    
    # 4. Base64 decode
    try:
        # Add padding if necessary
        missing_padding = len(line) % 4
        if missing_padding:
            line += '=' * (4 - missing_padding)
        
        decoded = base64.b64decode(line).decode('utf-8')
        print(f"Decoded: {decoded}")
        
        # 5. Split by comma
        parts = decoded.split(',')
        print(f"Parts: {parts}")
        
        return decoded, parts
    except Exception as e:
        print(f"Error: {e}")
        return None, None

decode_thrillrig("7G9hvzfkQjE0LFMsMSxGMTR")
