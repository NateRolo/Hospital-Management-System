#include <stdio.h>
#include <string.h>
#include "patient.h"
#include "utils.h"

Patient patients[MAX_PATIENT_CAPACITY];
int totalPatients = IS_EMPTY;
int patientIDCounter = DEFAULT_ID;

void addPatientRecord()
{
    if(totalPatients >= MAX_PATIENT_CAPACITY)
    {
        printf("Max patient capacity reached!\n");
        return;
    }

    char patientName[MAX_PATIENT_NAME_LENGTH];
    int patientAge;
    char patientDiagnosis[MAX_DIAGNOSIS_LENGTH];
    int roomNumber;

    if(getPatientName(patientName) == IS_INVALID)
    {
        return;
    }

    if(getPatientAge(&patientAge) == IS_INVALID)
    {
        return;
    }

    if(getPatientDiagnosis(patientDiagnosis) == IS_INVALID)
    {
        return;
    }

    if(getRoomNumber(&roomNumber) == IS_INVALID)
    {
        return;
    }

    Patient newPatient = createNewPatient(patientName,
                                          patientAge,
                                          patientDiagnosis,
                                          roomNumber);

    patients[totalPatients] = newPatient;
    totalPatients++;
    patientIDCounter++;

    printf("Patient added successfully!\n");
    printPatientInfo(&patients[totalPatients - 1]);
}

void viewPatientRecords()
{
    if (totalPatients == 0)
    {
        printf("No Patients Admitted...\n");
        return;
    }

    printf("--- Patient Record ---\n");
    for(int i = 0; i < MAX_PATIENT_CAPACITY; i++)
    {
        if (patients[i].patientId != 0)
        {
            printPatientInfo(&patients[i]);
        }
    }
}

void searchPatientById()
{
    int id, index;

    printf("Enter A Patient Id: ");
    scanf("%d", &id);
    clearInputBuffer();
    index = patientExists(id);

    if (index == PATIENT_NOT_FOUND)
    {
        printf("Patient Does Not Exist!\n");
    } 
    else
    {
        printf("Patient ID: %d\n", patients[index].patientId);
        printf("Patient Name: %s\n", patients[index].name);
        printf("Age: %d\n", patients[index].age);
        printf("Diagnosis: %s\n", patients[index].diagnosis);
        printf("Room Number: %d\n", patients[index].roomNumber);
        printf("---------------------------------------\n");
    }
}

void dischargePatient()
{
    if(totalPatients == IS_EMPTY)
    {
        printf("No patients to discharge!\n");
        return;
    }

    int index = getPatientIndexForDischarge();
    
    if(index == PATIENT_NOT_FOUND)
    {
        printf("Patient is not in system.\n");
        return;
    }

    if(confirmDischarge(index))
    {
        removePatientFromSystem(index);
        printf("Patient has been discharged!\n");
    }
    else
    {
        printf("Patient discharge cancelled.\n");
    }
}

static int getPatientName(char patientName[])
{
    printf("Enter patient name:\n");
    fgets(patientName, MAX_PATIENT_NAME_LENGTH, stdin);
    patientName[strcspn(patientName, "\n")] = '\0';
    clearInputBuffer();
    
    return validatePatientName(patientName);
}

static int getPatientAge(int* patientAge)
{
    printf("Enter patient age:\n");
    scanf("%d", patientAge);
    clearInputBuffer();
    
    return validatePatientAge(*patientAge);
}

static int getPatientDiagnosis(char patientDiagnosis[])
{
    printf("Enter patient diagnosis:\n");
    fgets(patientDiagnosis, MAX_DIAGNOSIS_LENGTH, stdin);
    patientDiagnosis[strcspn(patientDiagnosis, "\n")] = '\0';
    
    return validatePatientDiagnosis(patientDiagnosis);
}

static int getRoomNumber(int* roomNumber)
{
    printf("Enter patient room:\n");
    scanf("%d", roomNumber);
    clearInputBuffer();
    
    return validateRoomNumber(*roomNumber);
}

static Patient createNewPatient(const char patientName[],
                                int patientAge,
                                const char patientDiagnosis[],
                                int roomNumber)
{
    Patient newPatient;
    newPatient.patientId = patientIDCounter;
    strcpy(newPatient.name, patientName);
    newPatient.age = patientAge;
    strcpy(newPatient.diagnosis, patientDiagnosis);
    newPatient.roomNumber = roomNumber;
    return newPatient;
}

static int getPatientIndexForDischarge()
{
    int patientId;
    printf("Enter ID of patient to discharge:\n");
    scanf("%d", &patientId);
    clearInputBuffer();
    return patientExists(patientId);
}

static int confirmDischarge(int patientIndex)
{
    char confirm;
    printf("Patient ID: %d\n", patients[patientIndex].patientId);
    printf("Patient Name: %s\n", patients[patientIndex].name);
    printf("Are you sure you want to discharge this patient? (y/n)\n");
    scanf("%c", &confirm);
    clearInputBuffer();
    return confirm == 'y';
}

static void removePatientFromSystem(int index)
{
    for(int i = index; i < MAX_PATIENT_CAPACITY - 1; i++)
    {
        patients[i] = patients[i + 1];
    }
    totalPatients--;
}

static int patientExists(int id)
{
    for (int i = 0; i < MAX_PATIENT_CAPACITY; i++)
    {
        if (patients[i].patientId == id)
        {
            return i;
        }
    }
    return PATIENT_NOT_FOUND;
}

static int checkRoomOccupancy(int roomNumber)
{
    for(int i = 0; i < MAX_ROOM_NUMBER + 1; i++)
    {
        if(patients[i].roomNumber == roomNumber)
        {
            return i;
        }
    }
    return -1;
}

static int validatePatientName(char patientName[])
{
    if(strlen(patientName) < MIN_PATIENT_NAME_LENGTH || 
       strlen(patientName) > MAX_PATIENT_NAME_LENGTH)
    {
        printf("Patient name must be between %d and %d characters long.\n", 
               MIN_PATIENT_NAME_LENGTH, MAX_PATIENT_NAME_LENGTH);
        return IS_INVALID;
    }
    return IS_VALID;
}

static int validatePatientAge(int patientAge)
{
    if (patientAge < MIN_AGE_YEARS || 
        patientAge > MAX_AGE_YEARS)
    {
        printf("Invalid age! Please enter a number between %d and %d.\n", 
               MIN_AGE_YEARS, MAX_AGE_YEARS);
        return IS_INVALID;  
    }
    return IS_VALID;
}

static int validatePatientDiagnosis(char patientDiagnosis[])
{
    if(strlen(patientDiagnosis) < MIN_DIAGNOSIS_LENGTH || 
       strlen(patientDiagnosis) > MAX_DIAGNOSIS_LENGTH)
    {
        printf("Patient diagnosis must be between %d and %d characters long.\n", 
               MIN_DIAGNOSIS_LENGTH, MAX_DIAGNOSIS_LENGTH);
        return IS_INVALID;
    }
    return IS_VALID;
}

static int validateRoomNumber(int roomNumber)
{
    if (roomNumber < MIN_ROOM_NUMBER || 
        roomNumber > MAX_ROOM_NUMBER)
    {
        printf("Invalid Room Number: Must be between %d and %d.\n", 
               MIN_ROOM_NUMBER, MAX_ROOM_NUMBER);
        return IS_INVALID;
    }
    return IS_VALID;
}

static void printPatientInfo(const Patient* patient)
{
    printf("Patient ID: %d\n", patient->patientId);
    printf("Patient Name: %s\n", patient->name);
    printf("Age: %d\n", patient->age);
    printf("Diagnosis: %s\n", patient->diagnosis);
    printf("Room Number: %d\n", patient->roomNumber);
    printf("---------------------------------------\n");
}