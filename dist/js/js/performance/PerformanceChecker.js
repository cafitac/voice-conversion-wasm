/**
 * PerformanceChecker - 성능 측정 유틸리티 (JavaScript 버전)
 * C++ PerformanceChecker와 동일한 계층적 구조 지원
 */
export class PerformanceChecker {
    constructor() {
        // Feature 노드 리스트
        this.features = [];

        // 현재 활성 Feature
        this.currentFeature = null;

        // 현재 활성 Function 스택 (중첩 지원)
        this.functionStack = [];

        // 전체 시작 시간
        this.totalStartTime = 0;
        this.totalDuration = 0;
    }

    /**
     * Feature 측정 시작
     * @param {string} featureName - Feature 이름
     */
    startFeature(featureName) {
        if (this.totalStartTime === 0) {
            this.totalStartTime = performance.now();
        }

        this.currentFeature = {
            feature: featureName,
            startTime: performance.now(),
            duration: 0,
            functions: []
        };
    }

    /**
     * Feature 측정 종료
     */
    endFeature() {
        if (!this.currentFeature) {
            console.warn('No active feature to end!');
            return;
        }

        const endTime = performance.now();
        this.currentFeature.duration = endTime - this.currentFeature.startTime;

        this.features.push(this.currentFeature);
        this.currentFeature = null;

        this.totalDuration = endTime - this.totalStartTime;
    }

    /**
     * Function 측정 시작
     * @param {string} functionName - Function 이름
     */
    startFunction(functionName) {
        const functionNode = {
            name: functionName,
            startTime: performance.now(),
            duration: 0,
            children: []
        };

        this.functionStack.push(functionNode);
    }

    /**
     * Function 측정 종료
     */
    endFunction() {
        if (this.functionStack.length === 0) {
            console.warn('No active function to end!');
            return;
        }

        const functionNode = this.functionStack.pop();
        const endTime = performance.now();
        functionNode.duration = endTime - functionNode.startTime;

        // startTime 제거 (출력에 불필요)
        delete functionNode.startTime;

        // 부모에 추가
        if (this.functionStack.length > 0) {
            // 중첩된 함수 - 부모 함수의 children에 추가
            const parent = this.functionStack[this.functionStack.length - 1];
            parent.children.push(functionNode);
        } else if (this.currentFeature) {
            // 최상위 함수 - 현재 Feature의 functions에 추가
            this.currentFeature.functions.push(functionNode);
        }
    }

    /**
     * 하위 호환성을 위한 start 메서드
     * Feature를 시작하거나 Function을 시작
     */
    start(label) {
        // Feature 이름 패턴 감지 (예: "pitch_total", "duration_total")
        if (label.endsWith('_total')) {
            this.startFeature(label);
        } else {
            this.startFunction(label);
        }
    }

    /**
     * 하위 호환성을 위한 end 메서드
     */
    end(label) {
        if (label.endsWith('_total')) {
            this.endFeature();
        } else {
            this.endFunction();
        }
    }

    /**
     * Feature 목록 반환
     */
    getFeatures() {
        return this.features;
    }

    /**
     * 전체 실행 시간 반환
     */
    getTotalDuration() {
        return this.totalDuration;
    }

    /**
     * 모든 측정 결과 초기화
     */
    reset() {
        this.features = [];
        this.currentFeature = null;
        this.functionStack = [];
        this.totalStartTime = 0;
        this.totalDuration = 0;
    }

    /**
     * 결과를 C++ PerformanceChecker와 동일한 JSON 구조로 반환
     * @returns {Object}
     */
    getReportJSON() {
        return {
            totalDuration: parseFloat(this.totalDuration.toFixed(3)),
            features: this.features.map(feature => ({
                feature: feature.feature,
                duration: parseFloat(feature.duration.toFixed(3)),
                functions: feature.functions.map(func => this._serializeFunctionNode(func))
            }))
        };
    }

    /**
     * FunctionNode를 재귀적으로 직렬화
     * @private
     */
    _serializeFunctionNode(node) {
        return {
            name: node.name,
            duration: parseFloat(node.duration.toFixed(3)),
            children: node.children.map(child => this._serializeFunctionNode(child))
        };
    }

    /**
     * 결과를 JSON 문자열로 반환
     * @returns {string}
     */
    getReportJSONString() {
        return JSON.stringify(this.getReportJSON(), null, 2);
    }

    /**
     * 결과를 CSV 문자열로 반환
     * @returns {string}
     */
    getReportCSV() {
        let csv = 'Feature,Function,Duration(ms)\n';

        this.features.forEach(feature => {
            csv += `${feature.feature},,${feature.duration.toFixed(3)}\n`;

            feature.functions.forEach(func => {
                this._addFunctionToCSV(csv, func, 1);
            });
        });

        return csv;
    }

    /**
     * FunctionNode를 CSV에 추가 (재귀)
     * @private
     */
    _addFunctionToCSV(csv, node, depth) {
        const indent = '  '.repeat(depth);
        csv += `,${indent}${node.name},${node.duration.toFixed(3)}\n`;

        node.children.forEach(child => {
            this._addFunctionToCSV(csv, child, depth + 1);
        });

        return csv;
    }

    /**
     * 평균 계산 (하위 호환성)
     * @param {string} label
     * @returns {number}
     */
    getAverage(label) {
        // Feature 검색
        const features = this.features.filter(f => f.feature === label);
        if (features.length > 0) {
            const sum = features.reduce((acc, f) => acc + f.duration, 0);
            return sum / features.length;
        }

        // Function 검색
        let totalDuration = 0;
        let count = 0;

        this.features.forEach(feature => {
            this._findFunctionByName(feature.functions, label, (func) => {
                totalDuration += func.duration;
                count++;
            });
        });

        return count > 0 ? totalDuration / count : 0;
    }

    /**
     * Function을 이름으로 검색 (재귀)
     * @private
     */
    _findFunctionByName(functions, name, callback) {
        functions.forEach(func => {
            if (func.name === name) {
                callback(func);
            }
            if (func.children.length > 0) {
                this._findFunctionByName(func.children, name, callback);
            }
        });
    }

    /**
     * 콘솔에 결과 출력
     */
    displayResults() {
        console.log('=== Performance Report ===');
        console.log(`Total Duration: ${this.totalDuration.toFixed(3)} ms`);
        console.log('');

        this.features.forEach(feature => {
            console.log(`Feature: ${feature.feature} (${feature.duration.toFixed(3)} ms)`);

            feature.functions.forEach(func => {
                this._displayFunctionNode(func, 1);
            });

            console.log('');
        });

        console.log('==========================');
    }

    /**
     * FunctionNode를 콘솔에 출력 (재귀)
     * @private
     */
    _displayFunctionNode(node, depth) {
        const indent = '  '.repeat(depth);
        console.log(`${indent}${node.name}: ${node.duration.toFixed(3)} ms`);

        node.children.forEach(child => {
            this._displayFunctionNode(child, depth + 1);
        });
    }

    /**
     * CSV 파일로 다운로드
     * @param {string} filename
     */
    exportCSV(filename = 'performance_report.csv') {
        const csv = this.getReportCSV();
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();

        URL.revokeObjectURL(url);
    }

    /**
     * JSON 파일로 다운로드
     * @param {string} filename
     */
    exportJSON(filename = 'performance_report.json') {
        const json = this.getReportJSONString();
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();

        URL.revokeObjectURL(url);
    }
}
