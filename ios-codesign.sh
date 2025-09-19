echo "Executable Path (EXECUTABLE_PATH): $EXECUTABLE_PATH"
echo "Built Products Directory: $BUILT_PRODUCTS_DIR"
echo "Platform : $PLATFORM_NAME"

if [ "$PLATFORM_NAME" == "iphonesimulator" ]; then
    exit 0
fi

##############################
# CONFIG - customize these
##############################

APP_PATH="${BUILT_PRODUCTS_DIR}/${FULL_PRODUCT_NAME}"
PROFILE_DIR="$HOME/Library/Developer/Xcode/UserData/Provisioning Profiles"
IDENTITY="Apple Development: yejneshwar@cucircuits.com (AR8CPAA7QK)"  # Use `security find-identity -v -p codesigning` to get this
ENTITLEMENTS_PATH="./entitlements.plist"
BUNDLE_ID=$PRODUCT_BUNDLE_IDENTIFIER

##############################
# Step 0: Auto-select matching provisioning profile
##############################

echo "[INFO] Searching for valid provisioning profile for: $BUNDLE_ID"
MATCHED_PROFILE=""
LATEST_DATE=0

for profile in "$PROFILE_DIR"/*.mobileprovision; do
    echo "Checking profile: $profile"

    # Extract profile plist
    security cms -D -i "$profile" > tmp_profile.plist 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "[WARN] Failed to parse: $profile"
        continue
    fi

    # Extract bundle ID from profile (last part of application-identifier)
    RAW_IDENTIFIER=$(/usr/libexec/PlistBuddy -c "Print :Entitlements:application-identifier" tmp_profile.plist 2>/dev/null)
    if [ -z "$RAW_IDENTIFIER" ]; then
        echo "[WARN] No application-identifier in profile: $profile"
        continue
    fi

    TEAM_ID=$(echo "$RAW_IDENTIFIER" | cut -d'.' -f1)
    PROFILE_BUNDLE_ID=$(echo "$RAW_IDENTIFIER" | cut -d'.' -f2-)

    echo " - Team ID: $TEAM_ID"
    echo " - App ID: $PROFILE_BUNDLE_ID"

    if [[ "$PROFILE_BUNDLE_ID" != "$BUNDLE_ID" ]]; then
        echo " - Skipped: bundle ID does not match ($PROFILE_BUNDLE_ID ≠ $BUNDLE_ID)"
        continue
    fi

    # Get expiration date
    EXPIRATION_DATE=$(/usr/libexec/PlistBuddy -c "Print :ExpirationDate" tmp_profile.plist 2>/dev/null)
    echo " - Expiration: $EXPIRATION_DATE"

    if [[ "$OSTYPE" == "darwin"* ]]; then
        EXPIRATION_EPOCH=$(date -j -f "%a %b %d %T %Z %Y" "$EXPIRATION_DATE" "+%s" 2>/dev/null)
    else
        EXPIRATION_EPOCH=$(date -d "$EXPIRATION_DATE" "+%s" 2>/dev/null)
    fi

    if [[ -z "$EXPIRATION_EPOCH" ]]; then
        echo " - Skipped: invalid expiration date"
        continue
    fi

    NOW_EPOCH=$(date "+%s")
    if [[ $EXPIRATION_EPOCH -le $NOW_EPOCH ]]; then
        echo " - Skipped: profile expired"
        continue
    fi

    echo " - Valid profile found"

    # Prefer the latest valid one
    if [[ $EXPIRATION_EPOCH -gt $LATEST_DATE ]]; then
        MATCHED_PROFILE="$profile"
        LATEST_DATE=$EXPIRATION_EPOCH
        echo " - Selected as best match so far"
    fi

done

rm -f tmp_profile.plist

if [[ -z "$MATCHED_PROFILE" ]]; then
    echo "[ERROR] No valid provisioning profile found for bundle ID: $BUNDLE_ID"
    echo "      - Make sure 'Automatically manage signing' is checked in 'Signing & Capabilities'"
    echo "      - Make sure 'Team' is set in 'Signing & Capabilities'"
    exit 1
fi

echo "[INFO] Selected provisioning profile: $MATCHED_PROFILE"

##############################
# Step 1: Extract Entitlements
##############################

echo "[INFO] Extracting entitlements from provisioning profile..."
security cms -D -i "$MATCHED_PROFILE" > profile.plist

/usr/libexec/PlistBuddy -x -c "Print :Entitlements" profile.plist > "$ENTITLEMENTS_PATH"

# Check for application-identifier
if ! grep -q "application-identifier" "$ENTITLEMENTS_PATH"; then
  echo "[ERROR] application-identifier missing from entitlements"
  exit 1
fi

##############################
# Step 2: Embed provisioning profile
##############################

echo "[INFO] Embedding provisioning profile..."
cp "$MATCHED_PROFILE" "$APP_PATH/embedded.mobileprovision"

##############################
# Step 3: Code sign the app
##############################

echo "[INFO] Signing app with identity: $IDENTITY"
codesign -f -s "$IDENTITY" --entitlements "$ENTITLEMENTS_PATH" "$APP_PATH"

##############################
# Step 4: Verify
##############################

echo "[INFO] Verifying code signature..."
codesign -v --deep --strict "$APP_PATH"
echo "[SUCCESS] App signed successfully."

##############################
# Cleanup
##############################

rm -f profile.plist
