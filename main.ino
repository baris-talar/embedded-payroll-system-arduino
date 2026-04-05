#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <MemoryFree.h> 

// Setting up for LCD
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
bool in_sync_phase = true;
int current_index_of_the_account = 0;  
unsigned long last_sync_time = 0;  

bool in_studentid_mode = false;      
unsigned long final_button_press_time = 0;  
unsigned long last_scroll_time = 0;  
int scroll_offset = 0;  

struct payroll_account {
    String id;
    int grade;
    String job_title;
    String pen_status = "PEN"; 
    float salary = 0.00;           
};

payroll_account accounts[10]; // Limiting account number to max 10 for memory management
int acc_count = 0;

// Functions 
bool adding_payroll_acc(String input);
bool setting_pen_status(String input);
bool change_acc_grade(String input);
bool update_acc_sal(String input);
bool change_acc_title(String input);
bool dell_acc(String input);
void acc_display(int index);
void scroling();
void select_button();
void scroll_job_title(payroll_account &account);

// Special character bits for up and down arrow for a better user interface
uint8_t arrow_up[8] = {
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00000
};

uint8_t arrow_down[8] = {
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b10101,
    0b01110,
    0b00100,
    0b00000
};

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.setBacklight(3); 

    lcd.createChar(0, arrow_up);
    lcd.createChar(1, arrow_down);
}

bool adding_payroll_acc(String input) {
    Serial.print("received command: ");
    Serial.println(input);

    if (input.startsWith("ADD-")) {
        input = input.substring(4); 
        int first_dash = input.indexOf('-');
        int second_dash = input.indexOf('-', first_dash + 1);

        if (first_dash == -1 || second_dash == -1) {
            Serial.println("ERROR: invalid ADD format");
            return false;
        }

        String id = input.substring(0, first_dash);
        String grade_str = input.substring(first_dash + 1, second_dash);
        String job_title = input.substring(second_dash + 1);

        Serial.print("ID: "); Serial.println(id);
        Serial.print("Grade: "); Serial.println(grade_str);
        Serial.print("Title: "); Serial.println(job_title);

        // Checking if the id is 7 characters
        if (id.length() != 7 || !id.equals(String(id.toInt()))) {
            Serial.println("ERROR: invalid ID format");
            return false;
        }

        // Check if ID exists
        for (int i = 0; i < acc_count; i++) {
            if (accounts[i].id == id) {
                Serial.println("ERROR: account with id exists");
                return false;
            }
        }

        // Checking Grade
        int grade = grade_str.toInt();
        if (grade < 1 || grade > 9) {
            Serial.println("ERROR: invalid grade");
            return false;
        }

        // Checking Job Title
        if (job_title.length() < 3 || job_title.length() > 17) {
            Serial.println("ERROR: invalid job title length");
            return false;
        }

        // Error correction for job title, checking character by character, because the code is not working properly
        for (int i = 0; i < job_title.length(); i++) {
            char c = job_title.charAt(i);
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) {
                Serial.println("ERROR: invalid character in job title: ");
                Serial.println(c);
                return false;
            }
        }

        // Create a new account
        payroll_account new_acc;
        new_acc.id = id;
        new_acc.grade = grade;
        new_acc.job_title = job_title;
        new_acc.salary = 0.00;

        // Addition of the new account to the array
        accounts[acc_count++] = new_acc;
        Serial.println("DONE!");
        return true;
    }
    Serial.println("ERROR: invalid command");
    return false;
}

// PST command
bool setting_pen_status(String input) {
    Serial.print("Received command: ");
    Serial.println(input);

    // Checking the input for PST function
    if (!input.startsWith("PST-")) {
        Serial.println("ERROR: invalid PST format");
        return false;
    }

    // Parse ID and pension status
    int first_dash = input.indexOf('-', 4); // Find the second dash after "PST-"
    if (first_dash == -1) {
        Serial.println("ERROR: invalid PST format");
        return false;
    }

    String id = input.substring(4, first_dash);
    String pen_status = input.substring(first_dash + 1);

    // Checking ID again
    if (id.length() != 7 || !id.equals(String(id.toInt()))) {
        Serial.println("ERROR: invalid ID format");
        return false;
    }

    // Changing the pension status
    if (pen_status != "PEN" && pen_status != "NPEN") {
        Serial.println("ERROR: invalid pension status");
        return false;
    }

    // Finding a specific account from its id number
    int acc_index = -1;
    for (int i = 0; i < acc_count; i++) {
        if (accounts[i].id == id) {
            acc_index = i;
            break;
        }
    }

    // Checking if the account actually exists
    if (acc_index == -1) {
        Serial.println("ERROR: account not found");
        return false;
    }

    payroll_account &account = accounts[acc_index];

    // Checking pension status
    if ((account.pen_status == "PEN" && pen_status == "PEN") ||
        (account.pen_status == "NPEN" && pen_status == "NPEN")) {
        Serial.println("ERROR: pension status not changed");
        return false;
    }

    // Updating the pension status
    account.pen_status = pen_status;
    Serial.println("DONE!");
    return true;
}

// GRD command
bool change_acc_grade(String input) {
    Serial.print("Received command: ");
    Serial.println(input);

    // Checking the input for GRD
    if (!input.startsWith("GRD-")) {
        Serial.println("ERROR: invalid GRD format");
        return false;
    }

    // Parse id and new job grade
    int first_dash = input.indexOf('-', 4); // Find the second dash after "GRD-"
    if (first_dash == -1) {
        Serial.println("ERROR: invalid GRD format");
        return false;
    }

    String id = input.substring(4, first_dash);
    String new_grade_str = input.substring(first_dash + 1);

    // Checking again if the id is 7 characters
    if (id.length() != 7 || !id.equals(String(id.toInt()))) {
        Serial.println("ERROR: invalid ID format");
        return false;
    }

    // Checking if the new job grade is a valid one
    int new_grade_acc = new_grade_str.toInt();
    if (new_grade_acc < 1 || new_grade_acc > 9) {
        Serial.println("ERROR: invalid job grade");
        return false;
    }

    // Finding a specific account by its id again
    int acc_index = -1;
    for (int i = 0; i < acc_count; i++) {
        if (accounts[i].id == id) {
            acc_index = i;
            break;
        }
    }

    if (acc_index == -1) {
        Serial.println("ERROR: account not found");
        return false;
    }

    payroll_account &account = accounts[acc_index];

    // Checks for the new job grade
    if (new_grade_acc <= account.grade) {
        Serial.println("ERROR: cannot modify to the same or lower job grade");
        return false;
    }

    account.grade = new_grade_acc;
    Serial.println("DONE!");
    return true;
}

// Function to handle the SAL command
bool update_acc_sal(String input) {
    Serial.print("Received command: ");
    Serial.println(input);

    // checking for SAL
    if (!input.startsWith("SAL-")) {
        Serial.println("ERROR: invalid SAL format");
        return false;
    }

    // new salary
    int first_dash = input.indexOf('-', 4); // Find the second dash after "SAL-"
    if (first_dash == -1) {
        Serial.println("ERROR: invalid SAL format");
        return false;
    }

    String id = input.substring(4, first_dash);
    String sal_str = input.substring(first_dash + 1);

    // checking if id is 7 chars
    if (id.length() != 7 || !id.equals(String(id.toInt()))) {
        Serial.println("ERROR: invalid ID format");
        return false;
    }

    float salary = sal_str.toFloat();
    if (salary < 0.00 || salary >= 100000.00) {
        Serial.println("ERROR: salary out of range");
        return false;
    }

    // making it 2 decimal places
    salary = round(salary * 100) / 100.0;

    // finding account
    int acc_index = -1;
    for (int i = 0; i < acc_count; i++) {
        if (accounts[i].id == id) {
            acc_index = i;
            break;
        }
    }

    if (acc_index == -1) {
        Serial.println("ERROR: account not found");
        return false;
    }

    payroll_account &account = accounts[acc_index];
    account.salary = salary;
    Serial.println("DONE!");
    return true;
}

// CJT command
bool change_acc_title(String input) {
    Serial.print("Received command: ");
    Serial.println(input);

    if (!input.startsWith("CJT-")) {
        Serial.println("ERROR: invalid CJT format");
        return false;
    }

    // new job title
    int first_dash = input.indexOf('-', 4); 
    if (first_dash == -1) {
        Serial.println("ERROR: invalid CJT format");
        return false;
    }

    String id = input.substring(4, first_dash);
    String job_title = input.substring(first_dash + 1);

    if (id.length() != 7 || !id.equals(String(id.toInt()))) {
        Serial.println("ERROR: invalid ID format");
        return false;
    }

    // Checking the job title length
    if (job_title.length() < 3 || job_title.length() > 17) {
        Serial.println("ERROR: invalid job title length");
        return false;
    }

    // Error handling for job title
    for (int i = 0; i < job_title.length(); i++) {
        char c = job_title.charAt(i);
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) {
            Serial.println("ERROR: invalid character in job title: ");
            Serial.println(c);
            return false;
        }
    }

    int acc_index = -1;
    for (int i = 0; i < acc_count; i++) {
        if (accounts[i].id == id) {
            acc_index = i;
            break;
        }
    }

    // checking if account already exists
    if (acc_index == -1) {
        Serial.println("ERROR: account not found");
        return false;
    }

    // updating job title
    payroll_account &account = accounts[acc_index];
    account.job_title = job_title;
    Serial.println("DONE!");
    return true;
}

// DEL command
bool dell_acc(String input) {
    Serial.print("Received command: ");
    Serial.println(input);

    if (!input.startsWith("DEL-")) {
        Serial.println("ERROR: invalid DEL format");
        return false;
    }

    String id = input.substring(4);

    // Checking for 7 digits again
    if (id.length() != 7 || !id.equals(String(id.toInt()))) {
        Serial.println("ERROR: invalid ID format");
        return false;
    }

    int acc_index = -1;
    for (int i = 0; i < acc_count; i++) {
        if (accounts[i].id == id) {
            acc_index = i;
            break;
        }
    }

    if (acc_index == -1) {
        Serial.println("ERROR: account not found");
        return false;
    }

    // account deletion
    for (int i = acc_index; i < acc_count - 1; i++) {
        accounts[i] = accounts[i + 1];
    }
    acc_count--;  

    Serial.println("DONE!");
    return true;
}

// LCD display
void acc_display(int index) {
    if (index < 0 || index >= acc_count) return;  // Safety check

    payroll_account &account = accounts[index];

    // Navigation
    char up_arrow = (index > 0) ? 0 : ' ';  
    char down_arrow = (index < acc_count - 1) ? 1 : ' ';  

    // setting up backlight colour
    if (account.pen_status == "PEN") {
        lcd.setBacklight(2);  
    } else {
        lcd.setBacklight(1);  
    }

    // displaying account information
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(up_arrow);
    lcd.print(account.grade);
    lcd.print(" ");
    lcd.print(account.pen_status);
    lcd.print(" ");
    lcd.print(account.salary, 2);  

    lcd.setCursor(0, 1);
    lcd.write(down_arrow);

    if (account.job_title.length() > 7) {
        scroll_job_title(account);
    } else {
        lcd.print(account.id);
        lcd.print(" ");
        lcd.print(account.job_title);
    }
}

// extra scrolling feature for long job titles
void scroll_job_title(payroll_account &account) {
    unsigned long current_time = millis();
    if (current_time - last_scroll_time >= 500) {  
        last_scroll_time = current_time;
        scroll_offset++;
        if (scroll_offset > account.job_title.length()) {
            scroll_offset = 0;
        }
    }
    String display_title = account.job_title.substring(scroll_offset);
    if (display_title.length() < 9) {
        display_title += " " + account.job_title.substring(0, 9 - display_title.length());
    }
    lcd.print(account.id);
    lcd.print(" ");
    lcd.print(display_title);
}

// buttons
void scroling() {
    if (lcd.readButtons() & BUTTON_UP) {
        if (current_index_of_the_account > 0) {
            current_index_of_the_account--;
            scroll_offset = 0;  
            acc_display(current_index_of_the_account);
            delay(200);  
        }
    }
    if (lcd.readButtons() & BUTTON_DOWN) {
        if (current_index_of_the_account < acc_count - 1) {
            current_index_of_the_account++;
            scroll_offset = 0;  
            acc_display(current_index_of_the_account);
            delay(200);  
        }
    }
}

// SELECT button
void select_button() {
    if (lcd.readButtons() & BUTTON_SELECT) {
        if (millis() - final_button_press_time > 500) {  
            final_button_press_time = millis();  

            in_studentid_mode = !in_studentid_mode;

            if (in_studentid_mode) {
                lcd.clear();
                lcd.setBacklight(5);  
                lcd.setCursor(0, 0);
                lcd.print("Student ID:");
                lcd.setCursor(0, 1);
                lcd.print("F238402");  

                // extra free ram feature
                lcd.setCursor(8, 1);
                lcd.print("RAM:");
                lcd.print(freeMemory());
                lcd.print("B");
            } else {
                lcd.setBacklight(7);  
                acc_display(current_index_of_the_account);  
            }
        }
    }
}

void loop() {
    if (in_sync_phase) {
        // starting phase
        if (millis() - last_sync_time >= 2000) {
            Serial.print("R");
            last_sync_time = millis();
        }

        // checking for BEGIN command
        if (Serial.available() > 0) {
            String input = Serial.readStringUntil('\n');
            if (input == "BEGIN") {
                lcd.setBacklight(7); 
                Serial.println("BASIC");
                in_sync_phase = false;  
            }
        }
    } else {
        // main phase
        if (Serial.available() > 0) {
            String input = Serial.readStringUntil('\n');
            if (input.startsWith("ADD-")) {
                adding_payroll_acc(input);
            } else if (input.startsWith("PST-")) {
                setting_pen_status(input);
            } else if (input.startsWith("GRD-")) {
                change_acc_grade(input);
            } else if (input.startsWith("SAL-")) {
                update_acc_sal(input);
            } else if (input.startsWith("CJT-")) {
                change_acc_title(input);
            } else if (input.startsWith("DEL-")) {
                dell_acc(input);
            } else {
                Serial.println("ERROR: invalid command");
            }
        }

        if (!in_studentid_mode) {  
            acc_display(current_index_of_the_account);
        }

        scroling();
        select_button();
    }
}












