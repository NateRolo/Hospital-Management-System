/*
 * Author: Arsh M, Nathan O
 * Date: Feb 6, 2025
 * Purpose: This file implements functions for managing patient records in a hospital system.
 */

#include "patient_management.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "patient_data.h"
#include "utils.h"

// Private constants
#define INITIAL_CAPACITY 1
#define IS_EMPTY 0
#define DEFAULT_ID 1

static const int PATIENT_NOT_FOUND        = -1;
static const int INVALID_ID               = 0;
static const int REMOVE_PATIENT_ARRAY_MAX = 49;
static const int NEXT_INDEX_OFFSET        = 1;
static const int ROOM_UNOCCUPIED          = -1;

// Global patient data
static PatientNode *patientHead      = NULL;
static int          totalPatients    = IS_EMPTY;
static int          patientIDCounter = DEFAULT_ID;

// Function prototypes for internal helper functions
static char        *getPatientName(char patientName[]);
static int          getPatientAge(int *patientAge);
static char        *getPatientDiagnosis(char patientDiagnosis[]);
static int          getRoomNumber(int *roomNumber);
static Patient     *getPatientToDischarge(void);
static int          confirmDischarge(Patient *patient);
static void         removePatientFromSystem(Patient *patient);
static Patient     *getPatientFromList(int id);
static void         writePatientToFile(Patient newPatient);
static void         updatePatientsFile(void);
static PatientNode *insertPatientAtEndOfList(PatientNode *head, Patient data);
static int          isRoomOccupiedInList(int roomNumber, PatientNode *head);
static int          computeNextPatientId(void);
static int          countPatientsByTimeframe(int timeframe);
static void         logRoomUsage(int roomNumber);
static void         clearBinaryFile(const char* fileName);
static int          countDischargedPatientsByTimeframe(int timeframe);

/*
 * Initializes the patient management system.
 * Attempts to load patient data from patients.dat file.
 */
void initializePatientSystem(void)
{
    clearMemory();

    FILE *file = fopen("patients.dat", "rb");

    if (file == NULL)
    {
        puts("Error reading patients.dat. Initializing with default setting.");
        initializePatientSystemDefault();
        return;
    }

    if (fgetc(file) == EOF)
    {
        fclose(file);
        puts("patients.dat is empty. Initializing with default setting.");
        initializePatientSystemDefault();
        return;
    }
    rewind(file);

    // Populate Linked List
    Patient tempPatient;
    int foundData = 0;

    while (fread(&tempPatient, sizeof(Patient), 1, file) == 1)
    {
        foundData = 1;
        patientHead = insertPatientAtEndOfList(patientHead, tempPatient);
        if (patientHead == NULL)
        {
            puts("Error: Unable to populate linked list from patients.dat.");
            fclose(file);
            initializePatientSystemDefault();
            return;
        }
        totalPatients++;
    }

    fclose(file);

    if (!foundData)
    {
        puts("Warning: patients.dat contained no valid patient records.");
        // Clear File If Only Invalid Data Found
        clearBinaryFile("patients.dat");
        initializePatientSystemDefault();
    }
    else
    {
        patientIDCounter = computeNextPatientId();
        puts("Patients successfully loaded from file.");
    }
}

/*
 * Clears the contents of a binary file by opening it in write mode.
 * This effectively erases all data in the file.
 *
 * fileName: The name of the file to clear
 */
static void clearBinaryFile(const char* fileName)
{
    FILE *clearFile = fopen(fileName, "wb");
    if (clearFile == NULL)
    {
        puts("Error: Unable to clear patients.dat.");
    }
    else
    {
        fclose(clearFile);
    }
}

/*
 * Initializes the patient management system with default settings.
 * Called when patients.dat cannot be read or is empty.
 */
void initializePatientSystemDefault(void)
{
    patientHead      = NULL;
    totalPatients    = IS_EMPTY;
    patientIDCounter = DEFAULT_ID;
    puts("Patient system initialized with default settings using linked list.");
}


/*
 * Adds a new patient record to the system after validating input fields.
 */
void addPatientRecord(void)
{
    char patientName[MAX_PATIENT_NAME_LENGTH];
    int  patientAge;
    char patientDiagnosis[MAX_DIAGNOSIS_LENGTH];
    int  roomNumber;

    getPatientName(patientName);
    getPatientAge(&patientAge);
    getPatientDiagnosis(patientDiagnosis);
    getRoomNumber(&roomNumber);

    // Create and store new patient record
    Patient newPatient = createPatient(patientName, patientAge, patientDiagnosis, roomNumber, patientIDCounter);
    patientHead        = insertPatientAtEndOfList(patientHead, newPatient);
    totalPatients++;
    patientIDCounter++;

    writePatientToFile(newPatient);

    printf("--- Patient Added ---\n");
    printPatient(newPatient);
}


/*
 * Displays all patient records stored in the system.
 */
void viewPatientRecords(void)
{
    if(patientHead == NULL)
    {
        puts("No patients admitted!");
        return;
    }

    PatientNode *current = patientHead;
    while(current != NULL)
    {
        printPatient(current->data);
        current = current->nextNode;
    }
}

/*
 * Searches for a patient by ID and displays their details if found.
 */
void searchPatientById(void)
{
    int id, index;

    if(patientHead == NULL)
    {
        puts("No patients admitted!");
        return;
    }

    printf("Enter A Patient Id: ");
    scanf("%d", &id);
    clearInputBuffer();

    PatientNode *current = patientHead;
    while(current != NULL)
    {
        if(current->data.patientId == id)
        {
            printPatient(current->data);
            return;
        }
        current = current->nextNode;
    }

    puts("Patient doesn't exist!");
}

/*
 * Removes a patient from the system if they exist and discharge is confirmed.
 */
void dischargePatient(void)
{
    if(patientHead == NULL)
    {
        puts("No patients to discharge!");
        return;
    }

    Patient *patientToDischarge = getPatientToDischarge();

    if(patientToDischarge == NULL)
    {
        puts("Patient not found!");
        return;
    }

    if(confirmDischarge(patientToDischarge))
    {
        // Save discharged patient data
        DischargedPatient dischargedPatient;
        dischargedPatient.patient       = *patientToDischarge;
        dischargedPatient.dischargeDate = time(NULL); // Current time as discharge time

        // Append to discharged patients file
        FILE *file = fopen("discharged_patients.dat", "ab");
        if(file == NULL)
        {
            perror("Error opening discharged_patients.dat");            
            return;
        }

        if(fwrite(&dischargedPatient, sizeof(DischargedPatient), 1, file) != 1)
        {
            perror("Error writing to discharged_patients.dat");
            fclose(file);
            return;
        }
        fclose(file);

        logRoomUsage(patientToDischarge->roomNumber); // Log the room usage

        // Remove from the active patient list
        removePatientFromSystem(patientToDischarge); // Pass the pointer
        printf("Patient has been discharged!\n");
    }
    else
    {
        printf("Patient discharge cancelled.\n");
    }
}

/*
 * Creates a backup of current patient records to patients.dat file.
 */
void backupPatientSystem()
{
    updatePatientsFile();
}

/*
 * Restores patient records from patients.dat file.
 */
void restoreDataFromFile()
{
    initializePatientSystem();
}

/*
 * Frees allocated memory for patient data.
 */
void clearMemory()
{
    while(patientHead != NULL)
    {
        PatientNode *temp = patientHead;
        patientHead       = patientHead->nextNode;
        free(temp);
    }
    patientHead      = NULL;
    totalPatients    = IS_EMPTY;
    patientIDCounter = DEFAULT_ID;
}

/*
 * Prints a formatted patient report to both console and file.
 *
 * Parameters:
 *   file: Output file stream (must be open)
 *   header: Report title text
 *   result: Total patient count for the report
 *   timeframe: Time period filter (1=Daily, 2=Weekly, 3=Monthly)
 *
 * Formats and displays the report header, patient count, and detailed
 * information for each matching patient. Output is mirrored to both
 * console and the specified file.
 */
void printFormattedReport(FILE *file, const char *header, int result, int timeframe)
{
    // Get current time and format it as YYYY-MM-DD
    time_t     now         = time(NULL);
    struct tm *currentTime = localtime(&now);
    char       currentTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d", currentTime);

    // Print report header to console
    printf("%s - %s\n", header, currentTimeStr);
    printf("=======================================\n");
    printf("Total patients admitted: %d\n", result);
    printf("---------------------------------------\n");

    // Print same header to file
    fprintf(file, "%s - %s\n", header, currentTimeStr);
    fprintf(file, "=======================================\n");
    fprintf(file, "Total patients admitted: %d\n", result);
    fprintf(file, "---------------------------------------\n");

    if(result == 0)
    {
        // Handle case when no patients match the timeframe
        printf("| No patients admitted in this timeframe |\n");
        printf("---------------------------------------\n");

        fprintf(file, "| No patients admitted in this timeframe |\n");
        fprintf(file, "---------------------------------------\n");
    }
    else
    {
        PatientNode *patient = patientHead;
        struct tm   *admissionTime;
        char         admissionDateStr[20];

        while(patient != NULL)
        {
            // Get admission timestamp and convert to struct tm
            time_t admissionTimestamp = patient->data.admissionDate;
            admissionTime             = localtime(&admissionTimestamp);

            // Calculate time difference in hours between now and admission
            double secondsDiff = difftime(now, admissionTimestamp);
            int    hoursDiff   = secondsDiff / 3600;

            // Determine if patient falls within requested timeframe
            int past24Hours = (hoursDiff <= 24);  // Within last 24 hours
            int sameWeek    = (admissionTime->tm_year == currentTime->tm_year &&
                            (currentTime->tm_yday - admissionTime->tm_yday) < 7);  // Same week (within 7 days)
            int sameMonth =
                    (admissionTime->tm_year == currentTime->tm_year && 
                     admissionTime->tm_mon == currentTime->tm_mon);  // Same month

            // Filter patients based on timeframe parameter (1=daily, 2=weekly, 3=monthly)
            if((timeframe == 1 && past24Hours) || (timeframe == 2 && sameWeek) || (timeframe == 3 && sameMonth))
            {
                // Format admission date as YYYY-MM-DD
                strftime(admissionDateStr, sizeof(admissionDateStr), "%Y-%m-%d", admissionTime);

                // Print patient details to console with formatted columns
                printf("| ID: %-5d Name: %-15s | Age: %-3d Room: %-5d Diagnosis: %-20s | Admitted: %-10s |\n",
                       patient->data.patientId,
                       patient->data.name,
                       patient->data.ageInYears,
                       patient->data.roomNumber,
                       patient->data.diagnosis,
                       admissionDateStr);
                printf("---------------------------------------\n");

                // Print same details to file with identical formatting
                fprintf(file,
                        "| ID: %-5d Name: %-15s | Age: %-3d Room: %-5d Diagnosis: %-20s | Admitted: %-10s |\n",
                        patient->data.patientId,
                        patient->data.name,
                        patient->data.ageInYears,
                        patient->data.roomNumber,
                        patient->data.diagnosis,
                        admissionDateStr);
                fprintf(file, "---------------------------------------\n");
            }

            patient = patient->nextNode;  // Move to next patient in list
        }
    }
}

/*
 * Generates and displays a report of admitted patients
 * based on the selected timeframe.
 */
void displayPatientReport(int choice)
{
    int result = countPatientsByTimeframe(choice);

    FILE *file = fopen("patient_reports.txt", "a");
    if(file == NULL)
    {
        printf("Error opening file for writing!\n");
        return;
    }

    fprintf(file, "\n");

    printFormattedReport(file, "   Patient Admission Report - Daily", result, choice);

    fclose(file);
    printf("\nReport successfully written to patient_reports.txt\n");
}

/*
 * Prints a formatted report of discharged patients within
 * the selected timeframe to both console and file.
 */
void printDischargedFormattedReport(FILE *file, const char *header, int result, int timeframe)
{
    // Get current time for report header
    time_t     now         = time(NULL);
    struct tm *currentTime = localtime(&now);
    char       currentTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%Y-%m-%d", currentTime);

    // Print report header to console
    printf("%s - %s\n", header, currentTimeStr);
    printf("=======================================\n");
    printf("Total patients discharged: %d\n", result);
    printf("---------------------------------------\n");

    // Print report header to file
    fprintf(file, "%s - %s\n", header, currentTimeStr);
    fprintf(file, "=======================================\n");
    fprintf(file, "Total patients discharged: %d\n", result);
    fprintf(file, "---------------------------------------\n");

    if(result == 0)
    {
        // Handle case when no patients were discharged in the timeframe
        printf("| No patients discharged in this timeframe |\n");
        printf("---------------------------------------\n");

        fprintf(file, "| No patients discharged in this timeframe |\n");
        fprintf(file, "---------------------------------------\n");
    }
    else
    {
        // Open discharged patients data file for reading
        FILE *fileRead = fopen("discharged_patients.dat", "rb");
        if(fileRead == NULL)
        {
            printf("Error opening file to read discharged patients.\n");
            return;
        }

        DischargedPatient dischargedPatient;
        // Read each discharged patient record
        while(fread(&dischargedPatient, sizeof(DischargedPatient), 1, fileRead) == 1)
        {
            // Format discharge date for display
            time_t     dischargeTimestamp = dischargedPatient.dischargeDate;
            struct tm *dischargeTime      = localtime(&dischargeTimestamp);
            char       dischargeDateStr[20];
            strftime(dischargeDateStr, sizeof(dischargeDateStr), "%Y-%m-%d", dischargeTime);

            // Calculate time difference for filtering
            double secondsDiff = difftime(now, dischargeTimestamp);
            int    hoursDiff   = secondsDiff / 3600;

            // Determine if patient falls within selected timeframe
            int past24Hours = (hoursDiff <= 24);  // Within last 24 hours
            int sameWeek    = (dischargeTime->tm_year == currentTime->tm_year &&
                            (currentTime->tm_yday - dischargeTime->tm_yday) < 7);  // Within last 7 days
            int sameMonth =
                    (dischargeTime->tm_year == currentTime->tm_year && 
                     dischargeTime->tm_mon == currentTime->tm_mon);  // Within current month

            // Filter discharged patients by timeframe (1=daily, 2=weekly, 3=monthly)
            if((timeframe == 1 && past24Hours) || (timeframe == 2 && sameWeek) || (timeframe == 3 && sameMonth))
            {
                // Print patient details to console with formatted columns
                printf("| ID: %-5d Name: %-15s | Age: %-3d Room: %-5d Diagnosis: %-20s | Discharged: %-10s |\n",
                       dischargedPatient.patient.patientId,
                       dischargedPatient.patient.name,
                       dischargedPatient.patient.ageInYears,
                       dischargedPatient.patient.roomNumber,
                       dischargedPatient.patient.diagnosis,
                       dischargeDateStr);
                
                // Print same patient details to file
                fprintf(file,
                        "| ID: %-5d Name: %-15s | Age: %-3d Room: %-5d Diagnosis: %-20s | Discharged: %-10s |\n",
                        dischargedPatient.patient.patientId,
                        dischargedPatient.patient.name,
                        dischargedPatient.patient.ageInYears,
                        dischargedPatient.patient.roomNumber,
                        dischargedPatient.patient.diagnosis,
                        dischargeDateStr);

                // Add separator after each patient entry
                printf("---------------------------------------\n");
                fprintf(file, "---------------------------------------\n");
            }
        }

        fclose(fileRead);
    }
}

/*
 * Generates and displays a report of discharged
 * patients based on the selected timeframe.
 */
void displayDischargedPatientReport(int choice)
{
    int result = countDischargedPatientsByTimeframe(choice);

    FILE *file = fopen("discharged_reports.txt", "a");
    if(file == NULL)
    {
        printf("Error opening file for writing!\n");
        return;
    }

    fprintf(file, "\n");

    printDischargedFormattedReport(file, "   Discharged Patient Report - Weekly", result, choice);

    fclose(file);
    printf("\nDischarge Report successfully written to discharged_reports.txt\n");
}

/*
 * Reads room_usage.txt and displays a usage report
 * showing how many times each valid room (1-50) was used.
 */
void displayRoomUsageReport(void)
{
    FILE *file = fopen("room_usage.txt", "r");
    int   roomCounts[50 + 1] = { 0 }; // Array to store counts (index 0 unused)
    int   roomNumber;
    int   totalEntries = 0;
    int   validEntries = 0;

    printf("\n--- Room Usage Report ---\n");

    if(file == NULL)
    {
        printf("Error opening room_usage.txt for reading");
        return;
    }

    // Read each room number from the file
    while(fscanf(file, "%d", &roomNumber) == 1)
    {
        totalEntries++;
        // Validate the room number and increment the count
        if(roomNumber >= 1 && roomNumber <= 50)
        {
            roomCounts[roomNumber]++;
            validEntries++;
        }
        else
        {
            fprintf(stderr, "Warning: Found invalid room number '%d' in room_usage.txt\n", roomNumber);
        }
    }

    fclose(file);

    printf("Room | Usage Count\n");
    printf("-----|------------\n");

    int roomsReported = 0;
    for(int i = 1; i <= 50; i++)
    {
        if(roomCounts[i] > 0)
        {
            printf("%-4d | %d\n", i, roomCounts[i]);
            roomsReported++;
        }
    }

    if(roomsReported == 0 && validEntries == 0)
    {
        printf("No valid room usage data found in the file.\n");
    }

    printf("-------------------------\n");
    printf("Total entries read: %d\n", totalEntries);
    printf("Valid rooms logged: %d\n", validEntries);
    printf("-------------------------\n");
}

/*
 * Reads and validates the patient's name from user input.
 */
static char *getPatientName(char patientName[])
{
    int isValid;

    do
    {
        printf("Enter Patient Name:\n");
        fgets(patientName, 101, stdin);
        patientName[strcspn(patientName, "\n")] = '\0';

        isValid = validatePatientName(patientName);

        if(isValid == IS_NOT_VALID)
        {
            printf("Invalid patient name. Please try again.\n");
        }
    }
    while(isValid == IS_NOT_VALID);

    return patientName;
}



/*
 * Reads and validates the patient's age from user input.
 */
static int getPatientAge(int *patientAge)
{
    int isValid;
    int scanResult;

    do
    {
        printf("Enter Patient Age:\n");
        scanResult = scanf("%d", patientAge);
        clearInputBuffer();

        if(scanResult != SUCCESSFUL_READ)
        {
            printf("Invalid input. Please enter a number.\n");
            isValid = IS_NOT_VALID;
            continue;
        }

        isValid = validatePatientAge(*patientAge);

        if(isValid == IS_NOT_VALID)
        {
            printf("Invalid patient age. Please try again.\n");
        }
    }
    while(isValid == IS_NOT_VALID);

    return IS_VALID;
}

/*
 * Reads and validates the patient's diagnosis from user input.
 */
static char *getPatientDiagnosis(char patientDiagnosis[])
{
    int isValid;

    do
    {
        printf("Enter Patient Diagnosis:\n");
        fgets(patientDiagnosis, MAX_DIAGNOSIS_LENGTH, stdin);
        patientDiagnosis[strcspn(patientDiagnosis, "\n")] = '\0';

        isValid = validatePatientDiagnosis(patientDiagnosis);

        if(isValid == IS_NOT_VALID)
        {
            printf("Invalid diagnosis. Please try again.\n");
        }
    }
    while(isValid == IS_NOT_VALID);

    return patientDiagnosis;
}

/*
 * Reads and validates the room number for the patient.
 */
static int getRoomNumber(int *roomNumber)
{
    int isValid;

    do
    {
        printf("Enter Patient Room:\n");
        if(scanf("%d", roomNumber) != SUCCESSFUL_READ)
        {
            printf("Invalid input. Please enter a number.\n");
            clearInputBuffer();
            isValid = IS_NOT_VALID;
            continue;
        }
        clearInputBuffer();

        // First check if the room number is in valid range
        isValid = validateRoomNumber(*roomNumber);

        if(isValid == IS_NOT_VALID)
        {
            printf("Invalid room number. Please try again.\n");
            continue;
        }

        // Check if the room is already occupied by traversing the linked list.
        if(isRoomOccupiedInList(*roomNumber, patientHead) != ROOM_UNOCCUPIED)
        {
            printf("Room already occupied. Please choose another room.\n");
            isValid = IS_NOT_VALID;
        }
    }
    while(isValid == IS_NOT_VALID);

    return IS_VALID;
}

/*
 * Prompts user for a patient ID and returns their index in the system.
 */
static Patient *getPatientToDischarge(void)
{
    int patientId;
    printf("Enter ID of patient to discharge:\n");
    scanf("%d", &patientId);
    clearInputBuffer();
    return getPatientFromList(patientId);
}

/*
 * Asks the user to confirm the discharge of a patient.
 */
static int confirmDischarge(Patient *patient)
{
    char confirm;
    printf("Patient ID: %d\n", patient->patientId);
    printf("Patient Name: %s\n", patient->name);
    printf("Are you sure you want to discharge this patient? (y/n)\n");
    scanf(" %c", &confirm);
    clearInputBuffer();
    return confirm == 'y';
}

/*
 * Removes a patient from the system by unlinking the corresponding node from the linked list.
 */
static void removePatientFromSystem(Patient *patient)
{
    if(patientHead == NULL || patient == NULL)
    {
        return;
    }

    PatientNode *current  = patientHead;
    PatientNode *prevNode = NULL;

    if(current != NULL && current->data.patientId == patient->patientId)
    {
        patientHead = current->nextNode;
        free(current);
        totalPatients--;
        updatePatientsFile();
        return;
    }

    // Search for the patient to remove
    while(current != NULL && current->data.patientId != patient->patientId)
    {
        prevNode = current;
        current  = current->nextNode;
    }

    // If patient was not found
    if(current == NULL)
    {
        return;
    }

    // Unlink the node and free memory
    prevNode->nextNode = current->nextNode;
    free(current);
    totalPatients--;

    updatePatientsFile();
}


/**
 * Rewrites the patients.dat file with current patient data.
 * Opens file in write binary mode and writes all active patient records.
 */
static void updatePatientsFile(void)
{
    FILE *pTemp;
    pTemp = fopen("patients.tmp", "wb");
    if(pTemp == NULL)
    {
        perror("Error creating temporary backup file");
        return; // Keep original patients.dat
    }

    PatientNode *current;
    int          write_error;

    current     = patientHead;
    write_error = 0;

    while(current != NULL)
    {
        if(fwrite(&(current->data), sizeof(Patient), 1, pTemp) != 1)
        {
            perror("Error writing patient to temporary file");
            write_error = 1;
            break; // Stop writing
        }
        current = current->nextNode;
    }

    if(fclose(pTemp) != 0)
    { // Also check fclose error
        perror("Error closing temporary backup file");
        write_error = 1;
    }

    if(!write_error)
    {
        // Only replace original if temp write was fully successful
        if(remove("patients.dat") != 0 && errno != ENOENT)
        {
            perror("Error removing old patients.dat");
        }
        if(rename("patients.tmp", "patients.dat") != 0)
        {
            perror("Error renaming temporary file to patients.dat");
        }
        else
        {
            puts("patients.dat updated successfully."); // Success message only after rename
        }
    }
    else
    {
        puts("Backup failed. Original patients.dat remains unchanged.");
        remove("patients.tmp"); // Clean up failed temp file
    }
}

/*
 * Prompts user for a patient ID and returns a pointer to the patient record in the linked list.
 */
static Patient *getPatientFromList(int id)
{
    if(patientHead == NULL)
    {
        return NULL;
    }

    PatientNode *current = patientHead;
    while(current != NULL)
    {
        if(current->data.patientId == id)
        {
            return &(current->data);
        }
        current = current->nextNode;
    }
    return NULL;
}

/**
 * Appends a single patient record to the patients.dat file.
 * Opens file in append binary mode, writes patient data, and handles errors.
 */
static void writePatientToFile(Patient newPatient)
{
    FILE *pPatients = fopen("patients.dat", "ab");

    if(pPatients == NULL)
    {
        puts("\nUnable to find patients.dat. Patient not added to file.");
        return;
    }

    fwrite(&newPatient, sizeof(Patient), 1, pPatients);
    fclose(pPatients);
    puts("\nPatient successfully added to file.\n");
}

/*
 * Checks if a room is currently occupied.
 */
static int isRoomOccupiedInList(int roomNumber, PatientNode *head)
{
    PatientNode *current = head;
    while(current != NULL)
    {
        if(current->data.roomNumber == roomNumber)
            return 1;
        current = current->nextNode;
    }
    return ROOM_UNOCCUPIED;
}

/*
 * Computes the next available patient ID by scanning all loaded records.
 * If no patients are loaded, it returns DEFAULT_ID.
 */
static int computeNextPatientId(void)
{
    int          maxId   = 0;
    PatientNode *current = patientHead;
    while(current != NULL)
    {
        if(current->data.patientId > maxId)
        {
            maxId = current->data.patientId;
        }
        current = current->nextNode;
    }
    return maxId + 1;
}

/*
 * Inserts a new patient node at the end of the linked list.
 */
static PatientNode *insertPatientAtEndOfList(PatientNode *head, Patient data)
{
    PatientNode *newNode = malloc(sizeof(PatientNode));
    if(newNode == NULL)
    {
        free(newNode);
        return NULL;
    }

    newNode->data     = data;
    newNode->nextNode = NULL;

    if(head == NULL)
    {
        return newNode;
    }

    PatientNode *current = head;
    while(current->nextNode != NULL)
    {
        current = current->nextNode;
    }
    current->nextNode = newNode;

    puts("Patient inserted at end of list.");
    return head;
}

/*
 * Counts the number of patients admitted within
 * the specified timeframe (daily, weekly, or monthly).
 */
static int countPatientsByTimeframe(int timeframe)
{
    if(patientHead == NULL)
    {
        printf("No patients admitted!\n");
        return 0;
    }

    int        count       = 0;
    time_t     now         = time(NULL);
    struct tm *currentTime = localtime(&now);

    // Extract current year, day of year, and month
    int currentYear      = currentTime->tm_year;
    int currentDayOfYear = currentTime->tm_yday;
    int currentMonth     = currentTime->tm_mon;

    PatientNode *current = patientHead;
    while(current != NULL)
    {
        time_t     admissionTimestamp = current->data.admissionDate;
        struct tm *admissionTime      = localtime(&admissionTimestamp);

        // Extract admission year, day of year, and month
        int admissionYear      = admissionTime->tm_year;
        int admissionDayOfYear = admissionTime->tm_yday;
        int admissionMonth     = admissionTime->tm_mon;

        // Calculate the difference in hours directly
        double secondsDiff = difftime(now, admissionTimestamp);
        int    hoursDiff   = (int) (secondsDiff / 3600);

        // Adjusted time conditions
        int past24Hours = (hoursDiff <= 24);
        int sameWeek    = (admissionYear == currentYear && (currentDayOfYear - admissionDayOfYear) < 7);
        int sameMonth   = (admissionYear == currentYear && admissionMonth == currentMonth);

        // Increment count based on timeframe
        if((timeframe == 1 && past24Hours) || // Daily
           (timeframe == 2 && sameWeek) ||    // Weekly
           (timeframe == 3 && sameMonth))     // Monthly
        {
            count++;
        }

        current = current->nextNode;
    }

    return count;
}

/*
 * Counts the number of discharged patients from a
 * binary file within the specified timeframe.
 */
static int countDischargedPatientsByTimeframe(int timeframe)
{
    FILE *file = fopen("discharged_patients.dat", "rb");
    if(file == NULL)
    {
        printf("No discharged patients found!\n");
        return 0;
    }

    int        count       = 0;
    time_t     now         = time(NULL);
    struct tm *currentTime = localtime(&now);

    DischargedPatient dischargedPatient;
    while(fread(&dischargedPatient, sizeof(DischargedPatient), 1, file) == 1)
    {
        time_t     dischargeTimestamp = dischargedPatient.dischargeDate;
        struct tm *dischargeTime      = localtime(&dischargeTimestamp);

        double secondsDiff = difftime(now, dischargeTimestamp);
        int    hoursDiff   = secondsDiff / 3600;

        int past24Hours = (hoursDiff <= 24);
        int sameWeek =
                (dischargeTime->tm_year == currentTime->tm_year && (currentTime->tm_yday - dischargeTime->tm_yday) < 7);
        int sameMonth =
                (dischargeTime->tm_year == currentTime->tm_year && dischargeTime->tm_mon == currentTime->tm_mon);

        if((timeframe == 1 && past24Hours) || (timeframe == 2 && sameWeek) || (timeframe == 3 && sameMonth))
        {
            count++;
        }
    }

    fclose(file);
    return count;
}



/*
 * Appends the given room number to the
 * room_usage.txt file for logging purposes.
 */
static void logRoomUsage(int roomNumber)
{
    FILE *file = fopen("room_usage.txt", "a"); // Open in append mode

    if(file == NULL)
    {
        perror("Error opening room_usage.txt for logging");
        // Decide if this is a critical error or just a warning.
        // For now, we'll print an error and continue.
        return;
    }

    // Write the room number followed by a newline
    fprintf(file, "%d\n", roomNumber);

    fclose(file);
    // Optional: Add a confirmation message here if desired
    // printf("Room %d usage logged.\n", roomNumber);
}



