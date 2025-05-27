from flask import Flask, request, send_file, render_template, send_from_directory, url_for
import os
import subprocess
import shutil
from werkzeug.utils import secure_filename
from functools import wraps

app = Flask(__name__)

# Configure folders with absolute paths
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
ALLOWED_EXTENSIONS = {'txt', 'cpp', 'h', 'py', 'html', 'css', 'js', 'json'}

# Create necessary directories
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size

def cleanup_after_request(response):
    """Clean up any temporary files after the response has been sent"""
    if hasattr(response, '_cleanup_files'):
        for filepath in response._cleanup_files:
            try:
                if os.path.exists(filepath):
                    os.remove(filepath)
                    print(f"Cleaned up file: {filepath}")
            except Exception as e:
                print(f"Error cleaning up {filepath}: {str(e)}")
    return response

app.after_request(cleanup_after_request)

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

def compile_programs():
    """Compile the C++ compression and decompression programs"""
    try:
        print(f"Current working directory: {os.getcwd()}")
        print(f"Files in directory: {os.listdir('.')}")
        
        # Compile compression program
        subprocess.run(['g++', '-o', 'compress.exe', 'compress.c++', '-std=c++17'], 
                      check=True, capture_output=True, text=True)
        print("Successfully compiled compression program")
        
        # Compile decompression program
        subprocess.run(['g++', '-o', 'decompress.exe', 'decompress.c++', '-std=c++17'], 
                      check=True, capture_output=True, text=True)
        print("Successfully compiled decompression program")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Compilation error: {e.stderr}")
        return False
    except Exception as e:
        print(f"Unexpected error during compilation: {str(e)}")
        return False

def find_output_file(base_filename, extension):
    """Search for output file in various possible locations"""
    possible_locations = [
        os.path.join(BASE_DIR, base_filename + extension),
        os.path.join(UPLOAD_FOLDER, base_filename + extension),
        os.path.join(os.getcwd(), base_filename + extension),
        # Add more possible locations
        os.path.join(os.getcwd(), os.path.basename(base_filename) + extension),
        os.path.join(UPLOAD_FOLDER, os.path.basename(base_filename) + extension)
    ]
    
    print(f"\nSearching for output file: {base_filename}{extension}")
    print(f"Current working directory: {os.getcwd()}")
    print(f"Base directory: {BASE_DIR}")
    print(f"Upload folder: {UPLOAD_FOLDER}")
    print("\nChecking these locations:")
    for location in possible_locations:
        print(f"Checking: {location}")
        if os.path.exists(location):
            print(f"Found output file at: {location}")
            return location
        else:
            print(f"Not found at: {location}")
    
    print("\nListing files in current directory:")
    print(os.listdir('.'))
    print("\nListing files in upload directory:")
    print(os.listdir(UPLOAD_FOLDER))
    
    print("\nOutput file not found in any expected location")
    return None

@app.route('/')
def home():
    return render_template('home.html')

@app.route('/compress', methods=['GET', 'POST'])
def compress():
    if request.method == 'GET':
        return render_template('compress.html')
    
    if 'file' not in request.files:
        return 'No file uploaded', 400
    
    file = request.files['file']
    if file.filename == '':
        return 'No file selected', 400
    
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        input_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        
        try:
            # Save uploaded file
            file.save(input_path)
            print(f"Saved input file to: {input_path}")
            
            # Run compression
            print(f"Running compression on: {input_path}")
            result = subprocess.run(['./compress.exe', input_path], 
                                 capture_output=True, text=True, check=True)
            print(f"Compression output: {result.stdout}")
            
            # Find the compressed file
            output_file = find_output_file(filename, '.compressed')
            if not output_file:
                return 'Compression failed - output file not found', 500
            
            # Move the file to uploads if it's not already there
            final_path = os.path.join(UPLOAD_FOLDER, os.path.basename(output_file))
            if output_file != final_path:
                shutil.move(output_file, final_path)
                print(f"Moved output file to: {final_path}")
            
            # Clean up input file immediately
            if os.path.exists(input_path):
                os.remove(input_path)
                print(f"Cleaned up input file: {input_path}")
            
            print(f"Sending compressed file: {final_path}")
            response = send_file(final_path, 
                               as_attachment=True,
                               download_name=os.path.basename(final_path))
            
            # Mark the output file for cleanup after the response is sent
            response._cleanup_files = [final_path]
            return response
            
        except subprocess.CalledProcessError as e:
            print(f"Compression error: {e.stderr}")
            # Clean up any remaining files
            for path in [input_path, final_path] if 'final_path' in locals() else [input_path]:
                if os.path.exists(path):
                    os.remove(path)
            return f'Compression error: {e.stderr}', 500
        except Exception as e:
            print(f"Unexpected error: {str(e)}")
            # Clean up any remaining files
            for path in [input_path, final_path] if 'final_path' in locals() else [input_path]:
                if os.path.exists(path):
                    os.remove(path)
            return f'Unexpected error: {str(e)}', 500

    return 'Invalid file type', 400

@app.route('/decompress', methods=['GET', 'POST'])
def decompress():
    if request.method == 'GET':
        return render_template('decompress.html')
    
    if 'file' not in request.files:
        return 'No file uploaded', 400
    
    file = request.files['file']
    if file.filename == '':
        return 'No file selected', 400
    
    filename = secure_filename(file.filename)
    if not filename.endswith('.compressed'):
        return 'Invalid file type - must be a .compressed file', 400
    
    input_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    base_filename = filename.replace('.compressed', '')
    
    try:
        # Save uploaded file
        file.save(input_path)
        print(f"\nDecompression Process:")
        print(f"1. Saved compressed file to: {input_path}")
        print(f"2. Base filename for output: {base_filename}")
        
        # Run decompression
        print(f"3. Running decompression command on: {input_path}")
        result = subprocess.run(['./decompress.exe', input_path], 
                             capture_output=True, text=True, check=True)
        print(f"4. Decompression command output:")
        print(result.stdout)
        if result.stderr:
            print(f"Decompression stderr:")
            print(result.stderr)
        
        # Find the decompressed file
        print("5. Searching for decompressed output file...")
        output_file = find_output_file(base_filename, '.decompressed')
        if not output_file:
            print("\nDebugging information:")
            print(f"Expected base filename: {base_filename}")
            print(f"Working directory contents: {os.listdir('.')}")
            print(f"Upload directory contents: {os.listdir(UPLOAD_FOLDER)}")
            return 'Decompression failed - output file not found', 500
        
        # Move the file to uploads if it's not already there
        final_path = os.path.join(UPLOAD_FOLDER, os.path.basename(output_file))
        if output_file != final_path:
            print(f"6. Moving output file from {output_file} to {final_path}")
            shutil.move(output_file, final_path)
        else:
            print("6. Output file already in correct location")
        
        # Clean up input file immediately
        if os.path.exists(input_path):
            os.remove(input_path)
            print(f"7. Cleaned up input file: {input_path}")
        
        print(f"8. Sending decompressed file: {final_path}")
        response = send_file(final_path,
                           as_attachment=True,
                           download_name=os.path.basename(final_path))
        
        # Mark the output file for cleanup after the response is sent
        response._cleanup_files = [final_path]
        return response
        
    except subprocess.CalledProcessError as e:
        print(f"\nDecompression process error:")
        print(f"Command output: {e.stdout if hasattr(e, 'stdout') else 'No stdout'}")
        print(f"Command stderr: {e.stderr if hasattr(e, 'stderr') else 'No stderr'}")
        # Clean up any remaining files
        for path in [input_path, final_path] if 'final_path' in locals() else [input_path]:
            if os.path.exists(path):
                os.remove(path)
        return f'Decompression error: {e.stderr}', 500
    except Exception as e:
        print(f"\nUnexpected error during decompression:")
        print(f"Error type: {type(e)}")
        print(f"Error message: {str(e)}")
        # Clean up any remaining files
        for path in [input_path, final_path] if 'final_path' in locals() else [input_path]:
            if os.path.exists(path):
                os.remove(path)
        return f'Unexpected error: {str(e)}', 500

if __name__ == '__main__':
    print("Starting server setup...")
    if compile_programs():
        print("\nServer is running!")
        print("Access the web interface at:")
        print("http://localhost:5000")
        print("http://127.0.0.1:5000")
        app.run(host='0.0.0.0', port=5000, debug=True) 